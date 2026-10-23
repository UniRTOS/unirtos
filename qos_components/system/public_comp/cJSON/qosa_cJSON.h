/*
  Copyright (c) 2009-2017 Dave Gamble and Q_cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef _QOSA_cJSON__h
#define _QOSA_cJSON__h

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#ifdef __cplusplus
extern "C"
{
#endif

#include "qosa_def.h"

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

    /* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

Q_CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
Q_CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
Q_CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way Q_CJSON_EXPORT_SYMBOLS does

*/

#define Q_CJSON_CDECL   __cdecl
#define Q_CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(Q_CJSON_HIDE_SYMBOLS) && !defined(Q_CJSON_IMPORT_SYMBOLS) && !defined(Q_CJSON_EXPORT_SYMBOLS)
#define Q_CJSON_EXPORT_SYMBOLS
#endif

#if defined(Q_CJSON_HIDE_SYMBOLS)
#define Q_CJSON_PUBLIC(type) type Q_CJSON_STDCALL
#elif defined(Q_CJSON_EXPORT_SYMBOLS)
#define Q_CJSON_PUBLIC(type) __declspec(dllexport) type Q_CJSON_STDCALL
#elif defined(Q_CJSON_IMPORT_SYMBOLS)
#define Q_CJSON_PUBLIC(type) __declspec(dllimport) type Q_CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define Q_CJSON_CDECL
#define Q_CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define Q_CJSON_PUBLIC(type) __attribute__((visibility("default"))) type
#else
#define Q_CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define Q_CJSON_VERSION_MAJOR 1
#define Q_CJSON_VERSION_MINOR 7
#define Q_CJSON_VERSION_PATCH 19

/* Q_cJSON Types: */
#define Q_cJSON_Invalid       (0)
#define Q_cJSON_False         (1 << 0)
#define Q_cJSON_True          (1 << 1)
#define Q_cJSON_NULL          (1 << 2)
#define Q_cJSON_Number        (1 << 3)
#define Q_cJSON_String        (1 << 4)
#define Q_cJSON_Array         (1 << 5)
#define Q_cJSON_Object        (1 << 6)
#define Q_cJSON_Raw           (1 << 7) /* raw json */

#define Q_cJSON_IsReference   256
#define Q_cJSON_StringIsConst 512

    /* The Q_cJSON structure: */
    typedef struct Q_cJSON
    {
        /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
        struct Q_cJSON *next;
        struct Q_cJSON *prev;
        /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
        struct Q_cJSON *child;

        /* The type of the item, as above. */
        int type;

        /* The item's string, if type==Q_cJSON_String  and type == Q_cJSON_Raw */
        char *valuestring;
        /* writing to valueint is DEPRECATED, use Q_cJSON_SetNumberValue instead */
        int valueint;
        /* The item's number, if type==Q_cJSON_Number */
        double valuedouble;

        /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
        char *string;
    } Q_cJSON;

    typedef struct Q_cJSON_Hooks
    {
        /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
        void *(Q_CJSON_CDECL *malloc_fn)(qosa_size_t sz);
        void(Q_CJSON_CDECL *free_fn)(void *ptr);
    } Q_cJSON_Hooks;

    typedef int cJSON_bool;

/* Limits how deeply nested arrays/objects can be before Q_cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef Q_CJSON_NESTING_LIMIT
#define Q_CJSON_NESTING_LIMIT 1000
#endif

/* Limits the length of circular references can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef Q_CJSON_CIRCULAR_LIMIT
#define Q_CJSON_CIRCULAR_LIMIT 10000
#endif

    /* returns the version of Q_cJSON as a string */
    Q_CJSON_PUBLIC(const char *) Q_cJSON_Version(void);

    /* Supply malloc, realloc and free functions to Q_cJSON */
    Q_CJSON_PUBLIC(void) Q_cJSON_InitHooks(Q_cJSON_Hooks *hooks);

    /* Memory Management: the caller is always responsible to free the results from all variants of Q_cJSON_Parse (with Q_cJSON_Delete) and Q_cJSON_Print (with stdlib free, Q_cJSON_Hooks.free_fn, or Q_cJSON_free as appropriate). The exception is Q_cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
    /* Supply a block of JSON, and this returns a Q_cJSON object you can interrogate. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_Parse(const char *value);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_ParseWithLength(const char *value, qosa_size_t buffer_length);
    /* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
    /* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match Q_cJSON_GetErrorPtr(). */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
    Q_CJSON_PUBLIC(Q_cJSON *)
    Q_cJSON_ParseWithLengthOpts(const char *value, qosa_size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);

    /* Render a Q_cJSON entity to text for transfer/storage. */
    Q_CJSON_PUBLIC(char *) Q_cJSON_Print(const Q_cJSON *item);
    /* Render a Q_cJSON entity to text for transfer/storage without any formatting. */
    Q_CJSON_PUBLIC(char *) Q_cJSON_PrintUnformatted(const Q_cJSON *item);
    /* Render a Q_cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
    Q_CJSON_PUBLIC(char *) Q_cJSON_PrintBuffered(const Q_cJSON *item, int prebuffer, cJSON_bool fmt);
    /* Render a Q_cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
    /* NOTE: Q_cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_PrintPreallocated(Q_cJSON *item, char *buffer, const int length, const cJSON_bool format);
    /* Delete a Q_cJSON entity and all subentities. */
    Q_CJSON_PUBLIC(void) Q_cJSON_Delete(Q_cJSON *item);

    /* Returns the number of items in an array (or object). */
    Q_CJSON_PUBLIC(int) Q_cJSON_GetArraySize(const Q_cJSON *array);
    /* Retrieve item number "index" from array "array". Returns QOSA_NULL if unsuccessful. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_GetArrayItem(const Q_cJSON *array, int index);
    /* Get item "string" from object. Case insensitive. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_GetObjectItem(const Q_cJSON *const object, const char *const string);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_GetObjectItemCaseSensitive(const Q_cJSON *const object, const char *const string);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_HasObjectItem(const Q_cJSON *object, const char *string);
    /* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when Q_cJSON_Parse() returns 0. 0 when Q_cJSON_Parse() succeeds. */
    Q_CJSON_PUBLIC(const char *) Q_cJSON_GetErrorPtr(void);

    /* Check item type and return its value */
    Q_CJSON_PUBLIC(char *) Q_cJSON_GetStringValue(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(double) Q_cJSON_GetNumberValue(const Q_cJSON *const item);

    /* These functions check the type of an item */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsInvalid(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsFalse(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsTrue(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsBool(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsNull(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsNumber(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsString(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsArray(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsObject(const Q_cJSON *const item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_IsRaw(const Q_cJSON *const item);

    /* These calls create a Q_cJSON item of the appropriate type. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateNull(void);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateTrue(void);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateFalse(void);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateBool(cJSON_bool boolean);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateNumber(double num);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateString(const char *string);
    /* raw json */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateRaw(const char *raw);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateArray(void);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateObject(void);

    /* Create a string where valuestring references a string so
 * it will not be freed by Q_cJSON_Delete */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateStringReference(const char *string);
    /* Create an object/array that only references it's elements so
 * they will not be freed by Q_cJSON_Delete */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateObjectReference(const Q_cJSON *child);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateArrayReference(const Q_cJSON *child);

    /* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateIntArray(const int *numbers, int count);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateFloatArray(const float *numbers, int count);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateDoubleArray(const double *numbers, int count);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_CreateStringArray(const char *const *strings, int count);

    /* Append item to the specified array/object. */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_AddItemToArray(Q_cJSON *array, Q_cJSON *item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_AddItemToObject(Q_cJSON *object, const char *string, Q_cJSON *item);
    /* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the Q_cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & Q_cJSON_StringIsConst) is zero before
 * writing to `item->string` */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_AddItemToObjectCS(Q_cJSON *object, const char *string, Q_cJSON *item);
    /* Append reference to item to the specified array/object. Use this when you want to add an existing Q_cJSON to a new Q_cJSON, but don't want to corrupt your existing Q_cJSON. */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_AddItemReferenceToArray(Q_cJSON *array, Q_cJSON *item);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_AddItemReferenceToObject(Q_cJSON *object, const char *string, Q_cJSON *item);

    /* Remove/Detach items from Arrays/Objects. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_DetachItemViaPointer(Q_cJSON *parent, Q_cJSON *const item);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_DetachItemFromArray(Q_cJSON *array, int which);
    Q_CJSON_PUBLIC(void) Q_cJSON_DeleteItemFromArray(Q_cJSON *array, int which);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_DetachItemFromObject(Q_cJSON *object, const char *string);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_DetachItemFromObjectCaseSensitive(Q_cJSON *object, const char *string);
    Q_CJSON_PUBLIC(void) Q_cJSON_DeleteItemFromObject(Q_cJSON *object, const char *string);
    Q_CJSON_PUBLIC(void) Q_cJSON_DeleteItemFromObjectCaseSensitive(Q_cJSON *object, const char *string);

    /* Update array items. */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_InsertItemInArray(Q_cJSON *array, int which, Q_cJSON *newitem); /* Shifts pre-existing items to the right. */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_ReplaceItemViaPointer(Q_cJSON *const parent, Q_cJSON *const item, Q_cJSON *replacement);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_ReplaceItemInArray(Q_cJSON *array, int which, Q_cJSON *newitem);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_ReplaceItemInObject(Q_cJSON *object, const char *string, Q_cJSON *newitem);
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_ReplaceItemInObjectCaseSensitive(Q_cJSON *object, const char *string, Q_cJSON *newitem);

    /* Duplicate a Q_cJSON item */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_Duplicate(const Q_cJSON *item, cJSON_bool recurse);
    /* Duplicate will create a new, identical Q_cJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
    /* Recursively compare two Q_cJSON items for equality. If either a or b is QOSA_NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
    Q_CJSON_PUBLIC(cJSON_bool) Q_cJSON_Compare(const Q_cJSON *const a, const Q_cJSON *const b, const cJSON_bool case_sensitive);

    /* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant,
 * but should point to a readable and writable address area. */
    Q_CJSON_PUBLIC(void) Q_cJSON_Minify(char *json);

    /* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or QOSA_NULL on failure. */
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddNullToObject(Q_cJSON *const object, const char *const name);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddTrueToObject(Q_cJSON *const object, const char *const name);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddFalseToObject(Q_cJSON *const object, const char *const name);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddBoolToObject(Q_cJSON *const object, const char *const name, const cJSON_bool boolean);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddNumberToObject(Q_cJSON *const object, const char *const name, const double number);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddStringToObject(Q_cJSON *const object, const char *const name, const char *const string);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddRawToObject(Q_cJSON *const object, const char *const name, const char *const raw);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddObjectToObject(Q_cJSON *const object, const char *const name);
    Q_CJSON_PUBLIC(Q_cJSON *) Q_cJSON_AddArrayToObject(Q_cJSON *const object, const char *const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define Q_cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
    /* helper for the Q_cJSON_SetNumberValue macro */
    Q_CJSON_PUBLIC(double) Q_cJSON_SetNumberHelper(Q_cJSON *object, double number);
#define Q_cJSON_SetNumberValue(object, number) ((object != QOSA_NULL) ? Q_cJSON_SetNumberHelper(object, (double)number) : (number))
    /* Change the valuestring of a Q_cJSON_String object, only takes effect when type of object is Q_cJSON_String */
    Q_CJSON_PUBLIC(char *) Q_cJSON_SetValuestring(Q_cJSON *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns cJSON_Invalid else it returns the new type*/
#define Q_cJSON_SetBoolValue(object, boolValue)                                                                                                                \
    ((object != QOSA_NULL && ((object)->type & (cJSON_False | cJSON_True)))                                                                                    \
         ? (object)->type = ((object)->type & (~(cJSON_False | cJSON_True))) | ((boolValue) ? cJSON_True : cJSON_False)                                        \
         : cJSON_Invalid)

/* Macro for iterating over an array or object */
#define Q_cJSON_ArrayForEach(element, array) for (element = (array != QOSA_NULL) ? (array)->child : QOSA_NULL; element != QOSA_NULL; element = element->next)

    /* malloc/free objects using the malloc/free functions that have been set with Q_cJSON_InitHooks */
    Q_CJSON_PUBLIC(void *) Q_cJSON_malloc(qosa_size_t size);
    Q_CJSON_PUBLIC(void) Q_cJSON_free(void *object);

#ifdef __cplusplus
}
#endif
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* _QOSA_cJSON__h */
