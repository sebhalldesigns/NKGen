/***************************************************************
**
** NanoKit Tool Header File
**
** File         :  parser.h
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-03-23 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen XML Parser
**
***************************************************************/

#ifndef PARSER_H
#define PARSER_H

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct NodeProperty
{
    struct NodeProperty* next; /* Pointer to the next property in the linked list */

    const char* key;
    const char* value;
} NodeProperty;

/* Generic tree node structure */
typedef struct TreeNode
{
    const char* className;      /* XML class*/
    const char* instanceName;   /* Name attribute */
    const char* content;        /* Content of the node, if any */

    NodeProperty* properties;   /* Linked list of properties */

    struct TreeNode* child; /* Pointer to the first child node */
    struct TreeNode* sibling; /* Pointer to the next sibling node */
} TreeNode;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

TreeNode* ParseFile(char* contents, size_t size, const char* moduleName);

#endif /* PARSER_H */