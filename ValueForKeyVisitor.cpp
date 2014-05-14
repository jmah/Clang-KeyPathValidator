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

  if (Sel == VFKPathSelector)
    Consumer->emitDiagnosticsForTypeAndKeyPath(E->getReceiverType(), E->getArg(0));
  else
    Consumer->emitDiagnosticsForTypeAndKey(E->getReceiverType(), E->getArg(0));

  return true;
}
