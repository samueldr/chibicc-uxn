#include "chibi.h"

Type *void_type  = &(Type){ TY_VOID, 1, 1, 0 };
Type *bool_type  = &(Type){ TY_BOOL, 1, 1, 0 };
Type *char_type  = &(Type){ TY_CHAR, 1, 1, 0 };
Type *uchar_type  = &(Type){ TY_CHAR, 1, 1, 1 };
// Type *short_type = &(Type){ TY_SHORT, 2, 2, 0 };
// Type *ushort_type = &(Type){ TY_SHORT, 2, 2, 1 };
Type *int_type   = &(Type){ TY_INT, 2, 2, 0 };
Type *uint_type  = &(Type){ TY_INT, 2, 2, 1 };
//Type *long_type  = &(Type){ TY_LONG, 4, 4, 0 };
//Type *ulong_type  = &(Type){ TY_LONG, 4, 4, 1 };

bool is_integer(Type *ty) {
  TypeKind k = ty->kind;
  return k == TY_BOOL || k == TY_CHAR || // k == TY_SHORT ||
         k == TY_INT; //  ||k == TY_LONG;
}

int align_to(int n, int align) {
  return (n + align - 1) & ~(align - 1);
}

static Type *new_type(TypeKind kind, int size, int align) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->size = size;
  ty->align = align;
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR, 2, 2);
  ty->base = base;
  return ty;
}

Type *array_of(Type *base, int len) {
  Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
  ty->base = base;
  ty->array_len = len;
  return ty;
}

Type *func_type(Type *return_ty) {
  Type *ty = new_type(TY_FUNC, 1, 1);
  ty->return_ty = return_ty;
  return ty;
}

Type *enum_type(void) {
  return new_type(TY_ENUM, 2, 1);
}

Type *struct_type(void) {
  Type *ty = new_type(TY_STRUCT, 0, 1);
  ty->is_incomplete = true;
  return ty;
}

// implements the C standard "integer promotions"
// (only handles type ranks <= int because we don't have long etc)
Type *promote(Type *type) {
  // everything smaller than int is promoted to signed int, even unsigned types
  if (type->kind < TY_INT) {
    return int_type;
  } else {
    return type;
  }
}
// implements the C standard "usual arithmetic conversions"
// (only handles type ranks <= int because we don't have long etc)
Type *uac(Type *type1, Type *type2) {
  type1 = promote(type1);
  type2 = promote(type2);
  if (type1->is_unsigned || type2->is_unsigned) {
    return uint_type;
  } else {
    return int_type;
  }
}

void add_type(Node *node) {
  if (!node || node->ty)
    return;

  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->els);
  add_type(node->init);
  add_type(node->inc);

  for (Node *n = node->body; n; n = n->next)
    add_type(n);
  for (Node *n = node->args; n; n = n->next)
    add_type(n);

  switch (node->kind) {
  case ND_PTR_DIFF:
  case ND_LOGOR:
  case ND_LOGAND:
  case ND_NOT:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    node->ty = int_type;
    return;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_MOD:
  case ND_BITAND:
  case ND_BITOR:
  case ND_BITXOR:
    node->ty = uac(node->lhs->ty, node->rhs->ty);
    return;
  case ND_SHL:
  case ND_SHR:
  case ND_BITNOT:
    node->ty = promote(node->lhs->ty);
    return;
  case ND_PTR_ADD:
  case ND_PTR_SUB:
  case ND_ASSIGN:
  case ND_PRE_INC:
  case ND_PRE_DEC:
  case ND_POST_INC:
  case ND_POST_DEC:
  case ND_ADD_EQ:
  case ND_PTR_ADD_EQ:
  case ND_SUB_EQ:
  case ND_PTR_SUB_EQ:
  case ND_MUL_EQ:
  case ND_DIV_EQ:
  case ND_MOD_EQ:
  case ND_SHL_EQ:
  case ND_SHR_EQ:
  case ND_BITAND_EQ:
  case ND_BITOR_EQ:
  case ND_BITXOR_EQ:
    node->ty = node->lhs->ty;
    return;
  case ND_VAR:
    node->ty = node->var->ty;
    return;
  case ND_TERNARY:
    if (is_integer(node->then->ty))
      node->ty = uac(node->then->ty, node->els->ty);
    else
      node->ty = node->then->ty;
    return;
  case ND_COMMA:
    node->ty = node->rhs->ty;
    return;
  case ND_MEMBER:
    node->ty = node->member->ty;
    return;
  case ND_ADDR:
    if (node->lhs->ty->kind == TY_ARRAY)
      node->ty = pointer_to(node->lhs->ty->base);
    else
      node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF: {
    if (!node->lhs->ty->base)
      error_tok(node->tok, "invalid pointer dereference");

    Type *ty = node->lhs->ty->base;
    if (ty->kind == TY_VOID)
      error_tok(node->tok, "dereferencing a void pointer");
    if (ty->kind == TY_STRUCT && ty->is_incomplete)
      error_tok(node->tok, "incomplete struct type");
    node->ty = ty;
    return;
  }
  case ND_STMT_EXPR: {
    Node *last = node->body;
    while (last->next)
      last = last->next;
    node->ty = last->ty;
    return;
  }
  default:
    // nothing to add
    return;
  }
}
