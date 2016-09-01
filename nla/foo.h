#ifndef FOO_H
#define FOO_H


static const struct foo {
  int a;
  int b;
} defaults = {
  2,
  3
};

typedef struct foo foo;

#endif
