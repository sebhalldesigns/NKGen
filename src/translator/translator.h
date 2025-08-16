/***************************************************************
**
** NanoKit Tool Header File
**
** File         :  translator.h
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-08-16 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen translator
**
***************************************************************/

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>

#include <parser/parser.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

bool ValidateTree(TreeNode* rootNode);

bool ValidateClass(const char* className);
bool ValidateProperty(const char* className, const char* propertyName);

#endif /* TRANSLATOR_H */