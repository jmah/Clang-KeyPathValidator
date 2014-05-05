//
// ValueForKeyVisitor.cpp
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "ValueForKeyVisitor.h"

using namespace clang;

bool ValueForKeyVisitor::VisitObjCMessageExpr(ObjCMessageExpr *E) {
  if (E->getNumArgs() != 1 || !E->isInstanceMessage())
    return true;

  Selector Sel = E->getSelector();
  if (Sel != VFKSelector && Sel != VFKPathSelector)
    return true;

  ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(E->getArg(0));
  if (!KeyPathLiteral)
    return true;

  bool path = (Sel == VFKPathSelector);
  StringRef KeyPathString = KeyPathLiteral->getString()->getString();
  const size_t KeyCount = 1 + (path ? KeyPathString.count('.') : 0);

  StringRef *Keys = new StringRef[KeyCount];
  if (path) {
    typedef std::pair<StringRef,StringRef> StringPair;
    size_t i = 0;
    for (StringPair KeyAndPath = KeyPathString.split('.');
        KeyAndPath.first.size() > 0;
        KeyAndPath = KeyAndPath.second.split('.'))
      Keys[i++] = KeyAndPath.first;
  } else {
    Keys[0] = KeyPathString;
  }

  QualType ObjType = E->getReceiverType();

  size_t Offset = path ? 2 : 0; // @"  (but whole string for non-path, TODO: Set to 2 and properly adjust end of range)
  for (size_t i = 0; i < KeyCount; i++) {
    StringRef Key = Keys[i];

    bool Valid = Consumer->CheckKeyType(ObjType, Key);
    if (!Valid) {
      SourceRange KeyRange = KeyPathLiteral->getSourceRange();
      SourceLocation KeyStart = KeyRange.getBegin().getLocWithOffset(Offset);
      KeyRange.setBegin(KeyStart);
      if (path)
        KeyRange.setEnd(KeyStart.getLocWithOffset(1));

      Compiler.getDiagnostics().Report(KeyStart, Consumer->KeyDiagID)
        << Key << ObjType->getPointeeType().getAsString()
        << KeyRange << E->getReceiverRange();
      break;
    }
    Offset += Key.size() + 1;
  }

  delete[] Keys;
  return true;
}
