
#ifndef CUBETREE_H
#define CUBETREE_H

typedef struct _TreeAttr {
  unsigned int symname; // must be a symbol reference
  Value  * val; // the attribute value
} TreeAttr;

typedef struct _TreeElem {
  struct _TreeAttr ** attr;
  unsigned int nb_attrs;

  unsigned int symname; // must be a symbol reference
  Value * val; // the element value

  struct _TreeElem ** child;
  unsigned int nb_children;

  struct _TreeElem *parent;
} TreeElem;

#endif CUBETREE_H
