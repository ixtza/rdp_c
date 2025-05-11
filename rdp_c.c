/**
 *
 * // TODO: look into topic-recursive descent parser
 *
 * this code is shit, by restricting the DS implementation using stack only
 * i've wasted one week of my unemployed time
 *
 * edge case:
 * char (*king)(const int (*b)(int a),int  a)
 * int (*signal(int, void (*fp)(int)))(int)
 * int (*f[])()
 *
 * dcl :
 *  - *dir-dcl
 * dir-dcl:
 *  - name
 *  - (dcl)
 *  - dir-dcl[]
 *  - dir-dcl()
 *  - dir-dcl(datatype params)
 * params:
 *  - name
 *  - *params -> prone to error due to the last element bieng discarded, thus early return is done witout disolving the last token
 *  - (params)
 *  - (*params)(datatype params,datatype params)
 *  - params[]
 *
 * this is an introduction to error stacking, where an error message is accumulated!
 * how to handle such that an error message only appears once ?
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 5000

enum { NAME,
       PARENS,
       BRACKETS };

const char* types[] = {"auto", "char", "const", "double", "extern", "float", "int", "long", "register", "short", "signed", "static", "unsigned", "void", "volatile"};
int ntypes = 15;

int isspectype(char*);
int bsearch(const char*[], int, int, char*);

char getch();
void ungetch(char);
void bufreset();

char* pop_datatype();
void push_datatype(char*);
void reset_datatype();

char* memalloc(int);
void memfree(char*);
void memreset();

int gettoken();
void processdefinition();

int dcl();
int dir_dcl();
int dir_dcl_param();

int getdatatype();

int tokentype;
char token[MAXSIZE];
char* name[MAXSIZE];
char* datatype[MAXSIZE];
char out[MAXSIZE];

int main(int argc, char const* argv[]) {
  int valid;
  char* temp_datatype;
  int f_type;
  while ((f_type = getdatatype()) != EOF) {
    valid = 0;
    while (tokentype != '\n') {
      out[0] = '\0';
      valid = dcl();
      if (tokentype != '\n') {
        memreset();
        bufreset();
        reset_datatype();
        printf("error : syntax error.\n");
      }
      if (f_type) {
        temp_datatype = pop_datatype();
        sprintf(out, "%s %s", out, temp_datatype);
      }
    }
    if (valid) printf("%s\n", out);
  }
  return 0;
}

int dcl() {
  int ns, type;
  ns = 0;
  for (ns = 0; gettoken() == '*';)
    ns++;
  if (!dir_dcl()) {
    return 0;
  }
  while (ns-- > 0) {
    strcat(out, " pointer to");
  }
  return 1;
}

int dir_dcl() {
  int type, token_size;
  char* p;
  if (tokentype == '(') {
    if (!dcl()) {
      return 0;
    }
    if (tokentype != ')') {
      out[0] = '\0';
      printf("error : missing ).\n");
      return 0;
    }
  } else if (tokentype == NAME) {
    int name_size = strlen(token);
    p = memalloc(name_size);
    p[name_size] = '\0';
    strcpy(p, token);
    sprintf(out, "%s :", p);
  } else {
    out[0] = '\0';
    tokentype = '\n';
    printf("error : expected name or (dcl).\n");
    return 0;
  }

  /**
   * bypasses closing parens by dir_dcl, immediate returns to true
   * while doing so, it discard the closing parens. returning to the
   * original dir_dcl
   *
   * dir_dcl emphesize that it everything that enters must be a opening
   * parens or a name.
   */
  char* temp_datatype;

  // extract datatype
  while (
      (type = gettoken()) == BRACKETS ||
      type == PARENS ||
      type == '(') {
    if (type == BRACKETS) {
      strcat(out, " array");
      strcat(out, token);
      strcat(out, " of");
    } else if (type == PARENS) {
      strcat(out, " function returning");
    } else if (type == '(') {
      strcat(out, " function with an argument(s) of");
      do {
        int f_type = getdatatype();
        gettoken();
        if (!dir_dcl_param()) {
          return 0;
        }
        if (f_type) {
          temp_datatype = pop_datatype();
          sprintf(out, "%s %s", out, temp_datatype);
        }
      } while (tokentype != ')' && strcat(out, ","));
      strcat(out, " returning");
      if (tokentype == '\n') {
        break;
      }
    }
  }
  return 1;
}

int dir_dcl_param() {
  int type, token_size, ns;
  char* p;
  char* temp_datatype;

  if (tokentype == NAME) {
    int name_size = strlen(token);
    p = memalloc(name_size);
    p[name_size] = '\0';
    strcpy(p, token);
    sprintf(out, "%s %s", out, p);
  } else if (tokentype == '*') {
    for (ns = 1; (type = gettoken()) == '*';)
      ns++;
    int prevtokentype = tokentype;
    if (tokentype != ')' && !dir_dcl_param()) {
      return 0;
    }
    while (tokentype == BRACKETS) {
      strcat(out, " array");
      strcat(out, token);
      strcat(out, " of");
      gettoken();
    }
    while (ns-- > 0) {
      strcat(out, " pointer to");
    }
    return 1;
  } else if (tokentype == '(') {
    for (ns = 0; (type = gettoken()) == '*';)
      ns++;
    if (tokentype != ')' && !dir_dcl_param()) {
      return 0;
    }
    if (tokentype != ')') {
      out[0] = '\0';
      printf("error : missing ).\n");
      return 0;
    }
    while (ns-- > 0) {
      strcat(out, " pointer to");
    }

    if ((type = gettoken()) == PARENS) {
      strcat(out, " function returning");
    } else if (type == '(') {
      strcat(out, " function with an argument(s) of");
      do {
        int f_type = getdatatype();
        gettoken();
        if (!dir_dcl_param()) {
          return 0;
        }
        if (f_type) {
          temp_datatype = pop_datatype();
          sprintf(out, "%s %s", out, temp_datatype);
        }
      } while (tokentype != ')' && strcat(out, ","));
      strcat(out, " returning");
    }
  } else if (tokentype == BRACKETS) {
    strcat(out, " array");
    strcat(out, token);
    strcat(out, " of");
  } else if (tokentype == ')') {
    return 1;
  }

  if (tokentype == ',') {
    while (ns-- > 0) {
      strcat(out, " pointer to");
    }
    return 1;
  }

  /**
   * This dissolves closing parens, which can cause premature parens decay.
   * No need to readded it to the buffer,
   * but handle special case like '*' and ',' on its own.
   */
  while ((type = gettoken()) == BRACKETS) {
    strcat(out, " array");
    strcat(out, token);
    strcat(out, " of");
  }
  while (ns-- > 0) {
    strcat(out, " pointer to");
  }
  return 1;
}

int gettoken() {
  char c;
  char* p = token;
  while ((c = getch()) == ' ' || c == '\t');
  if (c == '(') {
    if ((c = getch()) == ')') {
      strcpy(token, "()");
      return tokentype = PARENS;
    } else {
      ungetch(c);
      return tokentype = '(';
    }
  } else if (c == '[') {
    for (*p++ = c; (*p++ = getch()) != ']';);
    *p = '\0';
    return tokentype = BRACKETS;
  } else if (isalpha(c)) {
    for (*p++ = c; isalnum(c = getch());)
      *p++ = c;
    *p = '\0';
    ungetch(c);
    return tokentype = NAME;
  } else
    return tokentype = c;
}

/**
 * sick trick, aka overengineered bozo
 */
int getdatatype() {
  int f_type, f_name;
  char temp_type[MAXSIZE];
  char temp_name[MAXSIZE];
  char* p;
  f_type = 0;
  f_name = 0;
  while (gettoken() == NAME) {
    if (isspectype(token)) {
      // add to datatype
      sprintf(temp_type, f_type ? "%s %s\0" : "%s\0", f_type ? temp_type : token, token);
      f_type = 1;
    } else {
      // add to name
      int name_size = strlen(token);
      p = memalloc(name_size);
      strcpy(p, token);
      for (int i = name_size - 1; i >= 0; i--) {
        ungetch(p[i]);
      }
      break;
    }
  }
  if (tokentype == BRACKETS || tokentype == PARENS) {
    int name_size = strlen(token);
    for (int i = name_size - 1; i >= 0; i--) {
      ungetch(token[i]);
    }
  } else if (tokentype != NAME) {
    ungetch(tokentype);
  }
  if (f_type) {
    int type_size = strlen(temp_type);
    p = memalloc(type_size + 1);
    p[type_size] = '\0';
    strcpy(p, temp_type);
    push_datatype(p);
  }
  return f_type;
}

int isspectype(char* s) {
  return bsearch(types, 0, ntypes, s) == 1;
}
int bsearch(const char* arr[], int left, int right, char* s) {
  int mid;
  if (left > right) return 0;
  mid = (left + right) / 2;
  if (strcmp(s, arr[mid]) < 0) {
    return bsearch(arr, left, mid - 1, s);
  } else if (strcmp(s, arr[mid]) > 0) {
    return bsearch(arr, mid + 1, right, s);
  } else {
    return 1;
  }
}

// buffer input

char bufchar[MAXSIZE];
int pbufchar = 0;

char getch() {
  return pbufchar > 0 ? bufchar[--pbufchar] : getchar();
}

void ungetch(char c) {
  if (pbufchar < MAXSIZE)
    bufchar[pbufchar++] = c;
  else
    printf("error : out of memory buffer\n");
}

void bufreset() {
  pbufchar = 0;
}

// buffer datatype

int pdatatype = 0;

char* pop_datatype() {
  return pdatatype > 0 ? datatype[--pdatatype] : NULL;
}

void push_datatype(char* s) {
  if (pdatatype < MAXSIZE)
    datatype[pdatatype++] = s;
  else
    printf("error : out of datatype memory buffer\n");
}

void reset_datatype() {
  pbufchar = 0;
}

// buffer memory

char bufmem[MAXSIZE];
char* pmem = bufmem;

char* memalloc(int size) {
  if (bufmem + MAXSIZE - pmem >= size) {
    pmem += size;
    return pmem - size;
  } else {
    printf("error : out of memory");
    return NULL;
  }
}

void memfree(char* p) {
  if (bufmem + MAXSIZE > p && p >= bufmem) {
    pmem = p;
  } else {
    printf("error : invalid memory location");
  }
}
void memreset() {
  pmem = bufmem;
}
