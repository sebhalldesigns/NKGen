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

typedef void(*WriterFunction)(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

typedef struct
{
    const char* typeName;
    const char* codeName;
    WriterFunction declarationWriter;
    WriterFunction valueWriter;
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
    const char* constructorName;
    PropertyEntry* properties; /* ensure NULL terminated */

    struct ClassEntry* super;
} ClassEntry;


/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

void CallbackDeclarationWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void StringWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void FloatWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void DockPositionWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void StackOrientationWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

void ColorWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile);

static CodeType codeTypes[] = {
    [TYPE_STRING] = {"STRING", "const char*", NULL, StringWriter},
    [TYPE_FLOAT] = {"FLOAT", "float", NULL, FloatWriter},
    [TYPE_THICKNESS] = {"THICKNESS", "nkThickness_t", NULL, NULL},
    [TYPE_COLOR] = {"COLOR", "nkColor_t", NULL, ColorWriter},
    [TYPE_BOOLEAN] = {"BOOLEAN", "bool", NULL, NULL},
    [TYPE_DOCK_POSITION] = {"DOCK_POSITION", "nkDockPosition_t", NULL, DockPositionWriter},
    [TYPE_STACK_ORIENTATION] = {"STACK_ORIENTATION", "nkStackOrientation_t", NULL, StackOrientationWriter},
    [TYPE_GENERIC_CALLBACK] = {"GENERIC_CALLBACK", "ViewMeasureCallback_t", CallbackDeclarationWriter, NULL},
    [TYPE_BUTTON_CALLBACK] = {"BUTTON_CALLBACK", "ButtonCallback_t", CallbackDeclarationWriter, NULL},
};

static PropertyEntry nkWindowProperties[] = {
    { "Title", "title", TYPE_STRING },
    { "Width", "width", TYPE_FLOAT },
    { "Height", "height", TYPE_FLOAT },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkViewProperties[] = {
    { "Name", "name", TYPE_STRING },
    { "Width", "sizeRequest.width", TYPE_FLOAT },
    { "Height", "sizeRequest.height", TYPE_FLOAT },
    { "Margin", "margin", TYPE_THICKNESS },
    { "Background", "backgroundColor", TYPE_COLOR },
    { "DockPanel.Dock", "dockPosition", TYPE_DOCK_POSITION },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkDockViewProperties[] = {
    { "LastChildFill", "lastChildFill", TYPE_BOOLEAN },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkStackViewProperties[] = {
    { "Orientation", "orientation", TYPE_STACK_ORIENTATION },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkScrollViewProperties[] = {
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static PropertyEntry nkButtonProperties[] = {
    { "Content", "text", TYPE_STRING },
    { "Text", "text", TYPE_STRING },
    { "Content", "text", TYPE_STRING },
    { "Foreground", "foreground", TYPE_COLOR },
    { "Background", "background", TYPE_COLOR },
    { "Click", "onClick", TYPE_BUTTON_CALLBACK },
    { NULL, NULL, TYPE_STRING } /* NULL TERMINATION */
};

static ClassEntry classes[] = {
    {"Window", "nkWindow_t", "nkWindow_Create", nkWindowProperties, NULL},
    {"View", "nkView_t", "nkView_Create", nkViewProperties, NULL},
    {"DockPanel", "nkDockView_t", "nkDockView_Create", nkDockViewProperties, &classes[1]},
    {"StackPanel", "nkStackView_t", "nkStackView_Create", nkStackViewProperties, &classes[1]},
    {"ScrollViewer", "nkScrollView_t", "nkScrollView_Create", nkScrollViewProperties, &classes[1]},
    {"Button", "nkButton_t", "nkButton_Create", nkButtonProperties, &classes[1]},
    {NULL, NULL, NULL, NULL, TYPE_STRING} /* NULL TERMINATION */
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

const char* TranslateSuperConstructor(const char* className)
{
    for (size_t i = 0; classes[i].markupName != NULL; i++)
    {
        if (strcmp(classes[i].markupName, className) == 0)
        {
            return classes[i].constructorName;
        }
    }

    printf("Error: Unknown class '%s'\n", className);
    return "[UNKNOWN]";
}

PropertyType ResolvePropertyType(const char* className, const char* propertyName, bool *isInherited)
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
            *isInherited = false;
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
                *isInherited = true;
                return classEntry->super->properties[i].type;

            }
        }
    }

    printf("Error: Unknown property '%s' for class '%s'\n", propertyName, className);
    return TYPE_STRING;
}

const char* TranslatePropertyName(const char* className, const char* propertyName)
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
        return "[ERROR]";
    }

    //printf("Valid class: %s\n", className);

    for (size_t i = 0; classEntry->properties[i].markupName != NULL; i++)
    {
        //printf("Checking property '%s' for class '%s'\n", classEntry->properties[i].markupName, className);

        if (strcmp(classEntry->properties[i].markupName, propertyName) == 0)
        {
            //printf("Valid property '%s' for class '%s'\n", propertyName, className);
            return classEntry->properties[i].codeName;
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
                return classEntry->super->properties[i].codeName;
            }
        }
    }

    printf("Error: Unknown property '%s' for class '%s'\n", propertyName, className);
    return "[ERROR]";
}

void DeclareCallback(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    if (propertyType >= TYPE_GENERIC_CALLBACK)
    {
        CallbackDeclarationWriter(propertyType, propertyValue, outputBuffer, outputBufferSize, positionInFile);
    }
}

void WriteValue(PropertyType type, const char* value, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    if (type < sizeof(codeTypes) / sizeof(CodeType))
    {
        if (codeTypes[type].valueWriter)
        {
            codeTypes[type].valueWriter(type, value, outputBuffer, outputBufferSize, positionInFile);
        }
        else
        {
            *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
                "(%s)%s;\n",
                codeTypes[type].codeName,
                value
            );
        }
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


void StringWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
        "\"%s\";\n",
        propertyValue
    );
}

void FloatWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
        "(float)%s;\n",
        propertyValue
    );
}

void DockPositionWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    const char* position = NULL;

    if (strcmp(propertyValue, "Right") == 0)
    {
        position = "DOCK_POSITION_RIGHT";
    }
    else if (strcmp(propertyValue, "Top") == 0)
    {
        position = "DOCK_POSITION_TOP";
    }
    else if (strcmp(propertyValue, "Bottom") == 0)
    {
        position = "DOCK_POSITION_BOTTOM";
    }
    else 
    {
        if (strcmp(propertyValue, "Left") != 0)
        {
            printf("Error: Unknown dock position '%s'. Defaulting to left.\n", propertyValue);
        }

        position = "DOCK_POSITION_LEFT"; // Default to left if not recognized
    }

    *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
        "%s;\n",
        position
    );
}

void StackOrientationWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    const char* orientation = NULL;

    if (strcmp(propertyValue, "Vertical") == 0)
    {
        orientation = "STACK_ORIENTATION_VERTICAL";
    }
    else 
    {   

        if (strcmp(propertyValue, "Horizontal") != 0)
        {
            printf("Error: Unknown stack orientation '%s'. Defaulting to horizontal.\n", propertyValue);
        } 

        orientation = "STACK_ORIENTATION_HORIZONTAL"; // Default to left if not recognized
    }

    *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
        "%s;\n",
        orientation
    );
}


void ColorWriter(PropertyType propertyType, const char* propertyValue, char* outputBuffer, size_t outputBufferSize, size_t* positionInFile)
{
    const char* namedColor = NULL;

    if (strcmp(propertyValue, "Black") == 0)
    {
        namedColor = "NK_COLOR_BLACK";
    }
    else if (strcmp(propertyValue, "White") == 0)
    {
        namedColor = "NK_COLOR_WHITE";
    }
    else if (strcmp(propertyValue, "Red") == 0)
    {
        namedColor = "NK_COLOR_RED";
    }
    else if (strcmp(propertyValue, "Green") == 0)
    {
        namedColor = "NK_COLOR_GREEN";
    }
    else if (strcmp(propertyValue, "Blue") == 0)
    {
        namedColor = "NK_COLOR_BLUE";
    }
    else if (strcmp(propertyValue, "Yellow") == 0)
    {
        namedColor = "NK_COLOR_YELLOW";
    }
    else if (strcmp(propertyValue, "Cyan") == 0)
    {
        namedColor = "NK_COLOR_CYAN";
    }
    else if (strcmp(propertyValue, "Orange") == 0)
    {
        namedColor = "NK_COLOR_ORANGE";
    }
    else if (strcmp(propertyValue, "Magenta") == 0)
    {
        namedColor = "NK_COLOR_MAGENTA";
    }
    else if (strcmp(propertyValue, "Gray") == 0)
    {
        namedColor = "NK_COLOR_GRAY";
    }
    else if (strcmp(propertyValue, "LightGray") == 0)
    {
        namedColor = "NK_COLOR_LIGHT_GRAY";
    }
    else if (strcmp(propertyValue, "DarkGray") == 0)
    {
        namedColor = "NK_COLOR_DARK_GRAY";
    }
    else
    {
        namedColor = "NK_COLOR_TRANSPARENT";
    }

    *positionInFile += snprintf(outputBuffer + *positionInFile, outputBufferSize - *positionInFile,
        "%s;\n",
        namedColor
    );

}
