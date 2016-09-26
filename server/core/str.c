#include "str.h"


string * string_init(pool_t *p) {
	string  *str = NULL;
	
	str =(string  *) palloc(p, sizeof(string));
	
	str->val = NULL;
	str->len = 0;

	return str;
}

string * string_init_from_str(pool_t *p, char *b, size_t len) {
	string  *str = NULL;
	
	str =(string  *) palloc(p, sizeof(string));

	str->val = (void *)palloc(p, sizeof(char)*(len+1));
	str->len = len;
	strncpy(str->val, b, len);
    str->val[len] = 0;

	return str;
}

string * string_init_from_ptr(pool_t *p, char *ptr, size_t len) {
    string  *str = NULL;
	
	str =(string  *) palloc(p, sizeof(string));

    str->val = ptr;
    str->len = len;

    return str;
}


int string_compare(const string *s1, const string *s2){
	int len;

	if(s1->size == s2->size) return strncmp(s1->val, s2->val, s1->len);
    if(s1.len == 0) {
        return -1;
    }
    if(s2.len == 0) {
        return 1;
    }

	len = s1->len > s2->len?s2->len:s1->len;
	if(len > 1) len--;

	return (s1->len[0] - s2->len[0]);
	
}

void string_copy_str_n(const string *b, char *s1, int n){
	if(b->val == NULL || b->len == 0) {*s1 = 0;return;}


	strncpy(s1, b->val, n);
	s1[n] = 0;
}