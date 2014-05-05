//
// KeyPathValidationConsumer.cpp
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "KeyPathValidationConsumer.h"

using namespace clang;


void KeyPathValidationConsumer::cacheNSTypes() {
  TranslationUnitDecl *TUD = Context.getTranslationUnitDecl();

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSNumber"));
    if (R.size() > 0) {
      ObjCInterfaceDecl *NSNumberDecl = dyn_cast<ObjCInterfaceDecl>(R[0]);
      NSNumberPtrType = Context.getObjCObjectPointerType(Context.getObjCInterfaceType(NSNumberDecl));
    } }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSDictionary"));
    if (R.size() > 0)
      NSDictionaryInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSArray"));
    if (R.size() > 0)
      NSArrayInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSSet"));
    if (R.size() > 0)
      NSSetInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }

  { DeclContext::lookup_result R = TUD->lookup(&Context.Idents.get("NSOrderedSet"));
    if (R.size() > 0)
      NSOrderedSetInterface = dyn_cast<ObjCInterfaceDecl>(R[0]); }
}


bool KeyPathValidationConsumer::CheckKeyType(QualType &ObjTypeInOut, StringRef &Key) {
  if (isKVCContainer(ObjTypeInOut)) {
    ObjTypeInOut = Context.getObjCIdType();
    return true;
  }

  // Special case keys
  if (Key.equals("self"))
    return true; // leave ObjTypeInOut unchanged


  std::vector<const ObjCContainerDecl *> ContainerDecls;
  if (const ObjCObjectPointerType *ObjType = ObjTypeInOut->getAs<ObjCObjectPointerType>()) {
    if (const ObjCInterfaceDecl *Interface = ObjType->getInterfaceDecl()) {
      ContainerDecls.push_back(Interface);
    }
    std::copy(ObjType->qual_begin(), ObjType->qual_end(), std::back_inserter(ContainerDecls));
  }

  IdentifierInfo *ID = &Context.Idents.get(Key);
  Selector Sel = Context.Selectors.getNullarySelector(ID);
  StringRef IsKey = ("is" + Key.substr(0, 1).upper() + Key.substr(1)).str();
  Selector IsSel = Context.Selectors.getNullarySelector(&Context.Idents.get(IsKey));

  QualType Type;
  for (std::vector<const ObjCContainerDecl *>::iterator Decl = ContainerDecls.begin(), DeclEnd = ContainerDecls.end();
      Decl != DeclEnd; ++Decl) {
	// Call the "same" (textually) method on both protocols and interfaces, not declared by the superclass
	if (const ObjCProtocolDecl *ProtoDecl = dyn_cast<const ObjCProtocolDecl>(*Decl)) {
	  if (const ObjCMethodDecl *Method = ProtoDecl->lookupMethod(Sel, true)) {
		Type = Method->getResultType();
		break;
	  }
	  if (const ObjCMethodDecl *Method = ProtoDecl->lookupMethod(IsSel, true)) {
		Type = Method->getResultType();
		break;
	  }
	  if (const ObjCPropertyDecl *Property = ProtoDecl->FindPropertyDeclaration(ID))
		if (isKVCCollectionType(Property->getType())) {
		  Type = Property->getType();
		  break;
		}
	}
	if (const ObjCInterfaceDecl *InterfaceDecl = dyn_cast<const ObjCInterfaceDecl>(*Decl)) {
	  if (const ObjCMethodDecl *Method = InterfaceDecl->lookupMethod(Sel, true)) {
		Type = Method->getResultType();
		break;
	  }
	  if (const ObjCMethodDecl *Method = InterfaceDecl->lookupMethod(IsSel, true)) {
		Type = Method->getResultType();
		break;
	  }
	  if (const ObjCPropertyDecl *Property = InterfaceDecl->FindPropertyDeclaration(ID))
		if (isKVCCollectionType(Property->getType())) {
		  Type = Property->getType();
		  break;
		}
	}
  }
  if (Type.isNull())
    return false;

  if (Type->isObjCObjectPointerType())
	ObjTypeInOut = Type;
  else if (NSAPIObj->getNSNumberFactoryMethodKind(Type).hasValue())
	ObjTypeInOut = NSNumberPtrType;

  // TODO: Primitives to NSValue
  return true;
}

bool KeyPathValidationConsumer::isKVCContainer(QualType Type) {
  if (Type->isObjCIdType())
    return true;

  const ObjCInterfaceDecl *ObjInterface = NULL;
  if (const ObjCObjectPointerType *ObjPointerType = Type->getAsObjCInterfacePointerType())
    ObjInterface = ObjPointerType->getInterfaceDecl();

  // Foundation built-ins
  if (NSDictionaryInterface->isSuperClassOf(ObjInterface) ||
      NSArrayInterface->isSuperClassOf(ObjInterface) ||
      NSSetInterface ->isSuperClassOf(ObjInterface)||
      NSOrderedSetInterface->isSuperClassOf(ObjInterface))
    return true;

  // Check for attribute
  while (ObjInterface) {
    for (Decl::attr_iterator Attr = ObjInterface->attr_begin(), AttrEnd = ObjInterface->attr_end();
        Attr != AttrEnd; ++Attr) {
      if (AnnotateAttr *AA = dyn_cast<AnnotateAttr>(*Attr))
        if (AA->getAnnotation().equals("objc_kvc_container"))
          return true;
    }

    ObjInterface = ObjInterface->getSuperClass();
  }
  return false;
}

bool KeyPathValidationConsumer::isKVCCollectionType(QualType Type) {
  const ObjCInterfaceDecl *ObjInterface = NULL;
  if (const ObjCObjectPointerType *ObjPointerType = Type->getAsObjCInterfacePointerType())
    ObjInterface = ObjPointerType->getInterfaceDecl();

  return (NSArrayInterface->isSuperClassOf(ObjInterface) ||
		  NSSetInterface ->isSuperClassOf(ObjInterface)||
		  NSOrderedSetInterface->isSuperClassOf(ObjInterface));
}
