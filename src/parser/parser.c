/***************************************************************
**
** NanoKit Tool Source File
**
** File         :  parser.c
** Module       :  nkgen
** Author       :  SH
** Created      :  2025-03-23 (YYYY-MM-DD)
** License      :  MIT
** Description  :  nkgen XML Parser
**
***************************************************************/


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <xml/xml.h>

#include "parser.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

struct xml_string {
	uint8_t const* buffer;
	size_t length;
};

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static TreeNode* rootNode = NULL;
static size_t nodeCount = 0;

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void TraverseNode(struct xml_node* node, TreeNode* parent);
static TreeNode* CreateNode(const char* className, const char* content, TreeNode* parent);
static void AddAttributeToNode(TreeNode* node, const char* key, const char* value);

static void PrintNode(TreeNode* node, size_t depth);

static void FreeNode(TreeNode* node);

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

TreeNode* ParseFile(char* contents, size_t size, const char* moduleName)
{
    nodeCount = 0;

    struct xml_document* document = xml_parse_document(contents, size);

    if (!document) 
    {
        fprintf(stderr, "Error: Could not parse input file\n");
        exit(1);
    }

    struct xml_node* root = xml_document_root(document);
    
    if (root) 
    {
        TraverseNode(root, NULL);
    }

    xml_document_free(document, true);

    /* print out tree */

    PrintNode(rootNode, 0);

    return rootNode;  
}

void FreeFile(TreeNode* rootNode)
{
    if (!rootNode) return;

    FreeNode(rootNode);
    rootNode = NULL;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void TraverseNode(struct xml_node* node, TreeNode* parent)
{

    /* CLASS */

    const char* nodeClass = calloc(xml_string_length(xml_node_name(node)) + 1, 1);
    xml_string_copy(xml_node_name(node), nodeClass, xml_string_length(xml_node_name(node)));
    
    //for (int i = 0; i < depth; i++) printf("  ");
    //printf("Node: %s\n", nodeClass ? (char*)nodeClass : "(null)");

    /* CONTENT */

    const char* nodeContent = calloc(xml_string_length(xml_node_content(node)) + 1, 1);
    xml_string_copy(xml_node_content(node), nodeContent, xml_string_length(xml_node_content(node)));

    if (strlen(nodeContent) > 0)
    {
        //for (int i = 0; i < depth + 1; i++) printf("  ");
        //printf("Content: %s\n", nodeContent); 
    } 
    else
    {
        //printf("No content for node %s\n", nodeClass);
        free((void*)nodeContent);
        nodeContent = NULL;
    }

    TreeNode* newNode = CreateNode(nodeClass, nodeContent, parent);

    /* ATTRIBUTES */

    size_t attributesCount = xml_node_attributes(node);
    
    for (size_t i = 0; i < attributesCount; i++)
    {
        struct xml_string* attributeNameObject = xml_node_attribute_name(node, i);
        struct xml_string* attributeContentObject = xml_node_attribute_content(node, i);

        const char* attributeName = calloc(xml_string_length(attributeNameObject) + 1, 1);
        xml_string_copy(attributeNameObject, attributeName, xml_string_length(attributeNameObject));

        /* code to handle items with spaces in between */
        const char* attributeContentObjectString = attributeContentObject->buffer;
        
        size_t attributeContentLength = 0;

        while (attributeContentObjectString[attributeContentLength] != '\0' && attributeContentObjectString[attributeContentLength] != '\"')
        {
            attributeContentLength++;
        }

        const char* attributeContent = calloc(attributeContentLength + 1, 1);
        sprintf(attributeContent, "%.*s", (int)attributeContentLength, attributeContentObjectString);

        //for (int i = 0; i < depth; i++) printf("  ");
        //printf("Attribute: %s = %s\n", attributeName, attributeContent);

        AddAttributeToNode(newNode, attributeName, attributeContent);
    }

    /* Recurse into children */
    size_t child_count = xml_node_children(node);
    for (size_t i = 0; i < child_count; i++) 
    {
        struct xml_node* child = xml_node_child(node, i);
        TraverseNode(child, newNode);
    }

}

static TreeNode* CreateNode(const char* className, const char* content, TreeNode* parent)
{
    TreeNode* newNode = (TreeNode*)malloc(sizeof(TreeNode));
    newNode->className = className;
    newNode->instanceName = NULL; /* from attribute Name */
    newNode->properties = NULL; 
    newNode->child = NULL;
    newNode->sibling = NULL;
    newNode->prevSibling = NULL;

    newNode->parent = parent;

    if (nodeCount == 0)
    {
        newNode->instanceName = "super"; /* root node */
    }
    else
    {
        char* defaultInstanceName = (char*)malloc(strlen("child") + (sizeof(size_t) * 2) + 1);
        sprintf(defaultInstanceName, "child%zu", nodeCount);
        newNode->instanceName = defaultInstanceName;
    }


    if (parent == NULL)
    {
        rootNode = newNode;
    }
    else
    {
        /* add node to appropriate level */

        if (parent->child == NULL)
        {
            parent->child = newNode;
        }
        else
        {
            TreeNode* sibling = parent->child;
            while (sibling->sibling != NULL)
            {
                sibling = sibling->sibling;
            }

            sibling->sibling = newNode;
            newNode->prevSibling = sibling;
        }
       
    }

    if (content)
    {
        AddAttributeToNode(newNode, "Content", content);
    }

    //printf("Created node: %s %p\n", className, newNode);

    nodeCount++;

    return newNode;
}

static void AddAttributeToNode(TreeNode* node, const char* key, const char* value)
{
    if (!node || !key || !value) return;

    if (strcmp(key, "Name") == 0)
    {
        node->instanceName = value;
    }
    else
    {
        NodeProperty* newProperty = (NodeProperty*)malloc(sizeof(NodeProperty));
        newProperty->key = key;
        newProperty->value = value;
        newProperty->next = NULL;

        if (!node->properties)
        {
            node->properties = newProperty;
        }
        else 
        {
            NodeProperty* current = node->properties;
            while (current->next != NULL)
            {
                current = current->next;
            }

            current->next = newProperty;
        }

        //printf("Added property: %s = %s to class %s\n", key, value, node->className);
    }
}

static void PrintNode(TreeNode* node, size_t depth)
{
    if (!node) return;

    for (size_t i = 0; i < depth; i++) printf("  ");
    printf("Node: %s\n", node->className ? node->className : "(null)");

    if (node->instanceName)
    {
        for (size_t i = 0; i < depth + 1; i++) printf("  ");
        printf("Instance Name: %s\n", node->instanceName);
    }

    NodeProperty* property = node->properties;
    while (property)
    {
        for (size_t i = 0; i < depth + 1; i++) printf("  ");
        printf("Property: %s = %s\n", property->key, property->value);
        property = property->next;
    }

    if (node->child)
    {
        PrintNode(node->child, depth + 1);
    }

    if (node->sibling)
    {
        PrintNode(node->sibling, depth);
    }

}

static void FreeNode(TreeNode* node)
{
    if (!node) return;
    
    if (node->className) free((void*)node->className);
    if (node->instanceName) free((void*)node->instanceName);

    NodeProperty* property = node->properties;
    while (property)
    {
        NodeProperty* nextProperty = property->next;
        free((void*)property->key);
        free((void*)property->value);
        free(property);
        property = nextProperty;
    }

    if (node->child) FreeNode(node->child);
    if (node->sibling) FreeNode(node->sibling);
    free(node);
}