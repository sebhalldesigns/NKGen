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

typedef void(*PropertyConverter)(const char* markupValue, size_t outputSize, void* output);

typedef struct
{
    const char* typeName;
    PropertyConverter converter;
} CodeType;

typedef struct 
{
    const char* markupName;
    const char* codeName;
    const char* typeName;
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

static CodeType codeTypes[] = {
    {"STRING", NULL},
    {"FUNCTION", NULL},
    {"FLOAT", NULL},
    {"THICKNESS", NULL},
    {"COLOR", NULL},
    {"BOOLEAN", NULL},
    {"DOCK_POSITION", NULL},
    {NULL, NULL} /* NULL TERMINATION */
};

static PropertyEntry nkWindowProperties[] = {
    { "Title", "title", "STRING" },
    { "Width", "width", "FLOAT" },
    { "Height", "height", "FLOAT" },
    { NULL, NULL, NULL } /* NULL TERMINATION */
};

static PropertyEntry nkViewProperties[] = {
    { "Name", "name", "STRING" },
    { "Width", "sizeRequest.width,", "FLOAT" },
    { "Height", "sizeRequest.height", "FLOAT" },
    { "Margin", "margin", "THICKNESS" },
    { "BackgroundColor", "backgroundColor", "COLOR" },
    { "DockPanel.Dock", "dockPosition", "DOCK_POSITION" },
    { NULL, NULL, NULL } /* NULL TERMINATION */
};

static PropertyEntry nkDockViewProperties[] = {
    { "LastChildFill", "lastChildFill", "BOOLEAN" },
    { NULL, NULL, NULL } /* NULL TERMINATION */
};

static PropertyEntry nkButtonProperties[] = {
    { "Text", "text", "STRING" },
    { "Foreground", "foreground", "COLOR" },
    { "Background", "background", "COLOR" },
    { NULL, NULL, NULL } /* NULL TERMINATION */
};

static ClassEntry classes[] = {
    {"Window", "nkWindow", nkWindowProperties, NULL},
    {"View", "nkView_t", nkViewProperties, NULL},
    {"DockPanel", "nkDockView_t", nkDockViewProperties, &classes[1]}, // nkView_t
    {"Button", "nkButton_t", nkButtonProperties, &classes[1]},
    {NULL, NULL, NULL, NULL} /* NULL TERMINATION */
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
            printf("Valid class: %s\n", className);
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

    printf("Valid class: %s\n", className);

    for (size_t i = 0; classEntry->properties[i].markupName != NULL; i++)
    {
        printf("Checking property '%s' for class '%s'\n", classEntry->properties[i].markupName, className);

        if (strcmp(classEntry->properties[i].markupName, propertyName) == 0)
        {
            printf("Valid property '%s' for class '%s'\n", propertyName, className);
            return true;
        }
    }
    
    if (classEntry->super)
    {
        for (size_t i = 0; classEntry->super->properties[i].markupName != NULL; i++)
        {
            printf("Checking property '%s' for class '%s'\n", classEntry->super->properties[i].markupName, classEntry->super->markupName);

            if (strcmp(classEntry->super->properties[i].markupName, propertyName) == 0)
            {
                printf("Valid property '%s' for class '%s'\n", propertyName, classEntry->super->markupName);
                return true;
            }
        }
    }

    printf("Error: Unknown property '%s' for class '%s'\n", propertyName, className);
    return false;
}