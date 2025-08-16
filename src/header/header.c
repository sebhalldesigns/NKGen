/***************************************************************
**
** NanoKit Tool Source File
**
** File         :  header.c
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-03-23 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen header file writer
**
***************************************************************/


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <xml/xml.h>

#include <translator/translator.h>

#include "header.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static char* outputBuffer = NULL;
static size_t outputBufferSize = 0;
static size_t positionInFile = 0;

static char moduleNameBuffer[256];
static char moduleNameUpper[256];

static size_t objectsCount = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

void DefineObject(TreeNode* node);
void DefineCallbacks(TreeNode* node);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void WriteHeaderFile(const char* path, const char* moduleName, TreeNode* fileContents)
{   
    objectsCount = 0;

    if (outputBuffer != NULL) 
    {
        free(outputBuffer);
    }

    outputBufferSize = 1024 * 1024; // 1 MB
    outputBuffer = (char*)malloc(outputBufferSize);
    if (outputBuffer == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for output buffer\n");
        exit(1);
    }

    sprintf(moduleNameBuffer, "%s", moduleName);
    
    for (size_t i = 0; moduleName[i] != '\0'; i++) {
        moduleNameUpper[i] = (moduleName[i] >= 'a' && moduleName[i] <= 'z') ? moduleName[i] - 32 : moduleName[i];
    }
    moduleNameUpper[strlen(moduleName)] = '\0';

    const char* moduleType = TranslateClassName(fileContents->className);

    /* BEGIN STRUCT DEFINITION */

    positionInFile = snprintf(outputBuffer, outputBufferSize, 
"/***************************************************************\n\
**\n\
** NanoKit Generated Header File\n\
**\n\
** File         :  %s\n\
** Module       :  %s\n\
**\n\
***************************************************************/\n\
\n\
#ifndef %s_XML_H\n\
#define %s_XML_H\n\
\n\
#include <nanowin.h>\n\
#include <views/views.h>\n\
\n\
typedef struct\n\
{\n\
    /* Base object */\n\
    %s super;\n\
\n\
    /* Child views */\n\
",
        path,
        moduleName,
        moduleNameUpper,
        moduleNameUpper,
        moduleType
    );

    TreeNode* currentNode = fileContents->child;

    while (currentNode != NULL)
    {
        DefineObject(currentNode);
        currentNode = currentNode->sibling;
    }

    /* END STRUCT DEFINITION */

   positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile, 
"} %s_t;\n\
\n\
/* Module Functions - Implementations Generated from XML */\n\
bool %s_Create(%s_t* this);\n\
void %s_Destroy(%s_t* this);\n\
\n\
/* Callback Functions - Implemented in User Code */\n\
",
        moduleName,
        moduleName,
        moduleName,
        moduleName,
        moduleName
    );

    DefineCallbacks(fileContents);

    /* CALLBACK DEFINITIONS */

    positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
        "\n\
#endif /*%s_XML_H*/\n",
        moduleNameUpper
    );

    FILE *headerFile = fopen(path, "w");
    
    if (!headerFile) {
        fprintf(stderr, "Error: Could not open header file for writing\n");
        return;
    }

    fprintf(headerFile, "%s", outputBuffer);
    fclose(headerFile);

    printf("    - Wrote header file: %s\n", path);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

void DefineObject(TreeNode* node)
{
    if (!node) return;

    if (node->instanceName)
    {
        positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
            "\t%s %s;\n",
            TranslateClassName(node->className),
            node->instanceName
        );
        
    }
    else
    {
        positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
            "\t%s %s%d;\n",
            TranslateClassName(node->className),
            "child",
            objectsCount
        );
    }

    objectsCount++;


    TreeNode* childNode = node->child;
    while (childNode != NULL)
    {
        DefineObject(childNode);
        childNode = childNode->sibling;
    }
}

void DefineCallbacks(TreeNode* node)
{
    if (!node) return;

    NodeProperty *property = node->properties;
    while (property != NULL)
    {
        PropertyType type = ResolvePropertyType(node->className, property->key);
        if (type >= TYPE_GENERIC_CALLBACK)
        {
            printf("Defining callback for property '%s' of type '%d'\n", property->key, type);
            DeclareCallback(type, property->value, outputBuffer, outputBufferSize, &positionInFile);
        }

        property = property->next;
    }

    TreeNode* childNode = node->child;
    while (childNode != NULL)
    {
        DefineCallbacks(childNode);
        childNode = childNode->sibling;
    }
}