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
#include <stdbool.h>

#include <parser/parser.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef enum
{
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_THICKNESS,
    TYPE_COLOR,
    TYPE_BOOLEAN,
    TYPE_DOCK_POSITION,

    /* ensure only callbacks after here as callback identified by >= TYPE_GENERIC_CALLBACK */
    TYPE_GENERIC_CALLBACK,
    TYPE_BUTTON_CALLBACK
} PropertyType;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

bool ValidateTree(TreeNode* rootNode);

bool ValidateClass(const char* className);
bool ValidateProperty(const char* className, const char* propertyName);

const char* TranslateClassName(const char* className);
const char* TranslateSuperConstructor(const char* className);
PropertyType ResolvePropertyType(const char* className, const char* propertyName, bool *isInherited);
const char* TranslatePropertyName(const char* className, const char* propertyName);

void DeclareCallback(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void WriteValue(PropertyType type, const char* value, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

#endif /* TRANSLATOR_H */