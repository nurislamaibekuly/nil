#include <ttml.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static double parse_time_seconds(const char* value) {
    int h = 0;
    int m = 0;
    double s = 0.0;
    int parsed = sscanf(value, "%d:%d:%lf", &h, &m, &s);
    if (parsed == 3) {
        return (double)h * 3600.0 + (double)m * 60.0 + s;
    }
    parsed = sscanf(value, "%d:%lf", &m, &s);
    if (parsed == 2) {
        return (double)m * 60.0 + s;
    }
    return atof(value);
}

static int has_element_child(xmlNode* node) {
    for (xmlNode* c = node->children; c; c = c->next) {
        if (c->type == XML_ELEMENT_NODE) return 1;
    }
    return 0;
}

static int span_is_background_self(xmlNode* span) {
    xmlChar* role = xmlGetNsProp(span, (const xmlChar*)"role", (const xmlChar*)"http://www.w3.org/ns/ttml#metadata");
    if (!role) role = xmlGetProp(span, (const xmlChar*)"role");
    int is_bg = (role && strcmp((char*)role, "x-bg") == 0);
    if (role) xmlFree(role);
    return is_bg;
}

static int span_has_background_ancestor(xmlNode* span) {
    for (xmlNode* n = span; n; n = n->parent) {
        if (n->type == XML_ELEMENT_NODE && strcmp((char*)n->name, "span") == 0) {
            if (span_is_background_self(n)) return 1;
        }
    }
    return 0;
}

static void collect_spans(xmlNode* node, Word** words, int* count, int* capacity) {
    for (xmlNode* cur = node; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE && strcmp((char*)cur->name, "span") == 0) {
            xmlChar* begin = xmlGetProp(cur, (const xmlChar*)"begin");
            xmlChar* end = xmlGetProp(cur, (const xmlChar*)"end");
            xmlChar* text = xmlNodeGetContent(cur);
            if (begin && end && text && text[0] != '\0' && !has_element_child(cur)) {
                int is_bg = span_has_background_ancestor(cur);
                if (*count >= *capacity) {
                    *capacity *= 2;
                    *words = realloc(*words, sizeof(Word) * (*capacity));
                }
                (*words)[*count].text = strdup((char*)text);
                (*words)[*count].start = parse_time_seconds((char*)begin);
                (*words)[*count].end = parse_time_seconds((char*)end);
                (*words)[*count].is_background = is_bg;
                (*count)++;
            }
            if (begin) xmlFree(begin);
            if (end) xmlFree(end);
            if (text) xmlFree(text);
        }
        if (cur->children) {
            collect_spans(cur->children, words, count, capacity);
        }
    }
}

Word* parse_ttml(const char* path, int* word_count) {
    xmlDoc *doc = xmlReadFile(path, NULL, 0);
    if (!doc) return NULL;

    xmlNode *root = xmlDocGetRootElement(doc);
    int capacity = 100;
    Word* words = malloc(sizeof(Word) * capacity);
    int count = 0;

    collect_spans(root, &words, &count, &capacity);

    *word_count = count;
    xmlFreeDoc(doc);
    return words;
}
