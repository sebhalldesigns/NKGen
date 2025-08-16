/***************************************************************
**
** NanoKit Tool Source File
**
** File         :  translator.c
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-08-16 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen translator
**
***************************************************************/


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xml/xml.h>

#include "translator.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef void(*DeclarationWriter)(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);
typedef void(*PropertyConverter)(const char* markupValue, size_t outputSize, void* output);

typedef struct
{
    const char* typeName;
    DeclarationWriter declarationWriter;
} CodeType;

typedef struct 
{
    const char* markupName;
    const char* codeName;
    PropertyType type;
} PropertyEntry;
 
typedef struct ClassEntry
{
    const char* markupName;
    const char* codeName;
    PropertyEntry* properties; /* ensure NULL terminated */

    struct ClassEntry* super;
} ClassEntry;


/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

void CallbackDeclarationWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

static CodeType codeTypes[] = {
    [TYPE_STRING] = {"STRING", NULL},
    [TYPE_FLOAT] = {"FLOAT", NULL},
    [TYPE_THICKNESS] = {"THICKNESS", NULL},
    [TYPE_COLOR] = {"COLOR", NULL},
    [TYPE_BOOLEAN] = {"BOOLEAN", NULL},
    [TYPE_DOCK_POSITION] = {"DOCK_POSITION", NULL},
    [TYPE_GENERIC_CALLBACK] = {"GENERIC_CALLBACK", CallbackDeclarationWriter},
    [TYPE_BUTTON_CALLBACK] = {"BUTTON_CALLBACK", CallbackDeclarationWriter},
};

static PropertyEntry nkWindowProperties[] = {
    { "Title", "title", TYPE_STRING },
    { "Width", "width", TYPE_FLOAT },
    { "Height", "height", TYPE_FLOAT },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkViewProperties[] = {
    { "Name", "name", TYPE_STRING },
    { "Width", "sizeRequest.width,", TYPE_FLOAT },
    { "Height", "sizeRequest.height", TYPE_FLOAT },
    { "Margin", "margin", TYPE_THICKNESS },
    { "BackgroundColor", "backgroundColor", TYPE_COLOR },
    { "DockPanel.Dock", "dockPosition", TYPE_DOCK_POSITION },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkDockViewProperties[] = {
    { "LastChildFill", "lastChildFill", TYPE_BOOLEAN },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkButtonProperties[] = {
    { "Text", "text", TYPE_STRING },
    { "Content", "text", TYPE_STRING },
    { "Foreground", "foreground", TYPE_COLOR },
    { "Background", "background", TYPE_COLOR },
    { "Click", "onClick", TYPE_BUTTON_CALLBACK },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static ClassEntry classes[] = {
    {"Window", "nkWindow_t", nkWindowProperties, NULL},
    {"View", "nkView_t", nkViewProperties, NULL},
    {"DockPanel", "nkDockView_t", nkDockViewProperties, &classes[1]}, // nkView_t
    {"Button", "nkButton_t", nkButtonProperties, &classes[1]},
    {NULL, NULL, NULL, TYPE_STRING} /* NULL TERMINATION */
};


/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool ValidateTree(TreeNode* rootNode)
{
    if (!rootNode) return false;

    if (!ValidateClass(rootNode->className))
    {
        return false;
    }

    NodeProperty* property = rootNode->properties;
    while (property)
    {
        if (!ValidateProperty(rootNode->className, property->key))
        {
            return false;
        }

        property = property->next;
    }

    if (rootNode->child)
    {
        if (!ValidateTree(rootNode->child))
        {
            return false;
        }
    }

    if (rootNode->sibling)
    {
        if (!ValidateTree(rootNode->sibling))
        {
            return false;
        }
    }

    return true;
}

bool ValidateClass(const char* className)
{
    for (size_t i = 0; classes[i].markupName != NULL; i++)
    {
        if (strcmp(classes[i].markupName, className) == 0)
        {
            //printf("Valid class: %s\n", className);
            return true;
        }
    }

    printf("Error: Unknown class '%s'\n", className);

    return false;
}

bool ValidateProperty(const char* className, const char* propertyName)
{

    ClassEntry* classEntry = NULL;

    for (size_t i = 0; classes[i].markupName != NULL; i++)
    {
        if (strcmp(classes[i].markupName, className) == 0)
        {
            classEntry = &classes[i];
            break;
        }
    }

    if (!classEntry)
    {
        printf("Error: Unknown class '%s'\n", className);
        return false;
    }

    //printf("Valid class: %s\n", className);

    for (size_t i = 0; classEntry->properties[i].markupName != NULL; i++)
    {
        //printf("Checking property '%s' for class '%s'\n", classEntry->properties[i].markupName, className);

        if (strcmp(classEntry->properties[i].markupName, propertyName) == 0)
        {
            //printf("Valid property '%s' for class '%s'\n", propertyName, className);
            return true;
        }
    }
    
    if (classEntry->super)
    {
        for (size_t i = 0; classEntry->super->properties[i].markupName != NULL; i++)
        {
            //printf("Checking property '%s' for class '%s'\n", classEntry->super->properties[i].markupName, classEntry->super->markupName);

            if (strcmp(classEntry->super->properties[i].markupName, propertyName) == 0)
            {
                //printf("Valid property '%s' for class '%s'\n", propertyName, classEntry->super->markupName);
                return true;
            }
        }
    }

    printf("Error: Unknown property '%s' for class '%s'\n", propertyName, className);
    return false;
}

const char* TranslateClassName(const char* className)
{
    for (size_t i = 0; classes[i].markupName != NULL; i++)
    {
        if (strcmp(classes[i].markupName, className) == 0)
        {
            return classes[i].codeName;
        }
    }

    printf("Error: Unknown class '%s'\n", className);
    return "[UNKNOWN]";
}

PropertyType ResolvePropertyType(const char* className, const char* propertyName)
{

    ClassEntry* classEntry = NULL;

    for (size_t i = 0; classes[i].markupName != NULL; i++)
    {
        if (strcmp(classes[i].markupName, className) == 0)
        {
            classEntry = &classes[i];
            break;
        }
    }

    if (!classEntry)
    {
        printf("Error: Unknown class '%s'\n", className);
        return TYPE_STRING;
    }

    //printf("Valid class: %s\n", className);

    for (size_t i = 0; classEntry->properties[i].markupName != NULL; i++)
    {
        //printf("Checking property '%s' for class '%s'\n", classEntry->properties[i].markupName, className);

        if (strcmp(classEntry->properties[i].markupName, propertyName) == 0)
        {
            //printf("Valid property '%s' for class '%s'\n", propertyName, className);
            return classEntry->properties[i].type;
        }
    }
    
    if (classEntry->super)
    {
        for (size_t i = 0; classEntry->super->properties[i].markupName != NULL; i++)
        {
            //printf("Checking property '%s' for class '%s'\n", classEntry->super->properties[i].markupName, classEntry->super->markupName);

            if (strcmp(classEntry->super->properties[i].markupName, propertyName) == 0)
            {
                //printf("Valid property '%s' for class '%s'\n", propertyName, classEntry->super->markupName);
                return classEntry->super->properties[i].type;

            }
        }
    }

    printf("Error: Unknown property '%s' for class '%s'\n", propertyName, className);
    return TYPE_STRING;
}

void DeclareCallback(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    if (propertyType >= TYPE_GENERIC_CALLBACK)
    {
        CallbackDeclarationWriter(propertyType, propertyValue, outputBuffer, outputBufferSize, positionInFile);
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

void CallbackDeclarationWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    switch (propertyType)
    {
        case TYPE_BUTTON_CALLBACK:
        {
            *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
                "void %s(nkButton_t *button);\n",
                propertyValue
            );
        } break;
        
        default:
        {
            *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
                "void %s();\n",
                propertyValue
            );
        } break;
    }
}

