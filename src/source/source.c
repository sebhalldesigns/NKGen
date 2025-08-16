/***************************************************************
**
** NanoKit Tool Source File
**
** File         :  source.c
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-03-23 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen source file writer
**
***************************************************************/


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <xml/xml.h>

#include <translator/translator.h>

#include "source.h"

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

static TreeNode* rootNode = NULL;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void InitialiseNode(TreeNode *node);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void WriteSourceFile(const char* path, const char* moduleName, TreeNode* fileContents)
{

    rootNode = fileContents;

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

    /* BEGIN FILE */

    positionInFile = snprintf(outputBuffer, outputBufferSize, 
"/***************************************************************\n\
**\n\
** NanoKit Generated Source File\n\
**\n\
** File         :  %s\n\
** Module       :  %s\n\
**\n\
***************************************************************/\n\
\n\
",
        path,
        moduleName
        );

    positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
        "\n\
#include \"%s.xml.h\"\n\
#include <stdio.h>\n\
\n", 
        moduleName
    );

    /* BEGIN CONSTRUCTOR */

    positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile, 
"/* Constructor */\n\
bool %s_Create(%s_t* this)\n\
{\n\
",
        moduleName,
        moduleName,
        moduleName,
        moduleName,
        moduleName
    );

    InitialiseNode(fileContents);

    /* END CONSTRUCTOR, BEGIN DESTRUCTOR */

    positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile, 
"}\n\
\n\
/* Destructor */\n\
void %s_Destroy(%s_t* this)\n\
{\n\
\n\
}\n\
",
        moduleName,
        moduleName
    );

    FILE *sourceFile = fopen(path, "w");
    
    if (!sourceFile) {
        fprintf(stderr, "Error: Could not open source file for writing\n");
        return;
    }

    fprintf(sourceFile, "%s", outputBuffer);
    fclose(sourceFile);

    printf("    - Wrote source file: %s\n", path);
}   


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void InitialiseNode(TreeNode *node)
{
    if (strcmp(node->className, "Window") == 0)
    {
        /* special case for window */
        float width = 800.0f;
        float height = 600.0f;
        const char* title = "NanoKit Window";

        NodeProperty* property = node->properties;
        while (property != NULL)
        {
            if (strcmp(property->key, "Width") == 0)
            {
                width = atof(property->value);
            }
            else if (strcmp(property->key, "Height") == 0)
            {
                height = atof(property->value);
            }
            else if (strcmp(property->key, "Title") == 0)
            {
                title = property->value;
            }

            property = property->next;
        }

        positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
            "\tnkWindow_Create(&this->%s, \"%s\", %.2f, %.2f);\n",
            node->instanceName,
            title,
            width,
            height
        );
    }
    else
    {
        positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
            "\t%s(&this->%s);\n",
            TranslateSuperConstructor(node->className),
            node->instanceName
        );
    }

    /* set attibutes */
    NodeProperty* property = node->properties;
    while (property != NULL)
    {
        bool isInherited = false;
        PropertyType type = ResolvePropertyType(node->className, property->key, &isInherited);

        if (isInherited)
        {
            positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
                "\tthis->%s.view.%s = ",
                node->instanceName,
                TranslatePropertyName(node->className, property->key)
            );
        }
        else
        {
            positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
                "\tthis->%s.%s = ",
                node->instanceName,
                TranslatePropertyName(node->className, property->key)
            );
        }

        WriteValue(type, property->value, outputBuffer, outputBufferSize, &positionInFile);

        property = property->next;
    }

    TreeNode* childNode = node->child;
    while (childNode != NULL)
    {
            positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
"\n\
\t/* Initialise %s */\n\
",
            childNode->instanceName
        );

        InitialiseNode(childNode);

        if (strcmp(node->className, "Window") == 0)
        {
            /* add to parent */
            positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
                "\n\tthis->super.rootView = (nkView_t *)&this->%s.view;\n",
                childNode->instanceName
            );
        }
        else
        {
            /* add to parent */
            positionInFile += snprintf(outputBuffer + positionInFile, outputBufferSize - positionInFile,
                "\n\tnkView_AddChildView(&this->%s.view, &this->%s.view);\n",
                node->instanceName,
                childNode->instanceName
            );
        }
        
        childNode = childNode->sibling;
    }
    
}