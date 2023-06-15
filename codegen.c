#include "chibi.h"

// static char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
// static char *argreg2[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
// static char *argreg4[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
// static char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static int brkseq;
static int contseq;
static char *funcname;
static bool need_sext_helper;
static bool need_ashr_helper;
static bool need_slt_helper;
static bool need_sle_helper;
static bool need_sdiv_helper;

static void gen(Node *node);

static void emit_add(int n) {
  switch (n) {
  case 0:
    return;
  case 1:
    printf("  INC2\n");
    return;
  case 2:
    printf("  INC2 INC2\n");
    return;
  case 3:
    printf("  INC2 INC2 INC2\n");
    return;
  default:
    printf("  #%04x ADD2\n", n);
    return;
  }
}

// Pushes the given node's address to the stack.
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    if (node->init)
      gen(node->init);

    Var *var = node->var;
    if (var->is_local) {
      printf("  STH2rk\n");
      emit_add(var->offset);
    } else {
      printf("  ;%s_\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen(node->lhs);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs);
    // printf("  pop rax\n");
    // printf("  add rax, %d\n", node->member->offset);
    // printf("  push rax\n");
    emit_add(node->member->offset);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

static void gen_lval(Node *node) {
  if (node->ty->kind == TY_ARRAY)
    error_tok(node->tok, "not an lvalue");
  gen_addr(node);
}

static void load(Type *ty) {
  // printf("  pop rax\n");

  // if (ty->size == 1) {
  //   printf("  movsx rax, byte ptr [rax]\n");
  // } else if (ty->size == 2) {
  //   printf("  movsx rax, word ptr [rax]\n");
  // } else if (ty->size == 4) {
  //   printf("  movsxd rax, dword ptr [rax]\n");
  // } else {
  //   assert(ty->size == 8);
  //   printf("  mov rax, [rax]\n");
  // }

  // printf("  push rax\n");
  if (ty->size == 1) {
    need_sext_helper = 1;
    printf("  LDA sext\n");
  } else {
    printf("  LDA2\n");
  }
}

// Expects "addr oldval newval" on the stack and leaves "oldval".
static void store_inner(Type *ty) {
  if (ty->size == 1) {
    if (ty->kind == TY_BOOL)
      printf("  #0000 NEQ2 #00 SWP\n");
    printf("  ROT2 STA POP\n");
  } else {
    printf("  ROT2 STA2\n");
  }
}

// Expects "addr newval" on the stack and leaves "newval".
static void store(Type *ty) {
  // printf("  pop rdi\n");
  // printf("  pop rax\n");

  // if (ty->kind == TY_BOOL) {
  //   printf("  cmp rdi, 0\n");
  //   printf("  setne dil\n");
  //   printf("  movzb rdi, dil\n");
  // }

  // if (ty->size == 1) {
  //   printf("  mov [rax], dil\n");
  // } else if (ty->size == 2) {
  //   printf("  mov [rax], di\n");
  // } else if (ty->size == 4) {
  //   printf("  mov [rax], edi\n");
  // } else {
  //   assert(ty->size == 8);
  //   printf("  mov [rax], rdi\n");
  // }

  // printf("  push rdi\n");
  printf("  DUP2\n");
  store_inner(ty);
}

static void truncate(Type *ty) {
  // printf("  pop rax\n");

  // if (ty->kind == TY_BOOL) {
  //   printf("  cmp rax, 0\n");
  //   printf("  setne al\n");
  // }

  // if (ty->size == 1) {
  //   printf("  movsx rax, al\n");
  // } else if (ty->size == 2) {
  //   printf("  movsx rax, ax\n");
  // } else if (ty->size == 4) {
  //   printf("  movsxd rax, eax\n");
  // }
  // printf("  push rax\n");
  if (ty->kind == TY_BOOL) {
    printf("  #0000 NEQ2 #00 SWP\n");
  } else if (ty->size == 1) {
    need_sext_helper = 1;
    printf("  NIP sext\n");
  }
}

static void inc(Type *ty) {
  // printf("  pop rax\n");
  // printf("  add rax, %d\n", ty->base ? ty->base->size : 1);
  // printf("  push rax\n");
  int n = ty->base ? ty->base->size : 1;
  emit_add(n);
}

static void dec(Type *ty) {
  // printf("  pop rax\n");
  // printf("  sub rax, %d\n", ty->base ? ty->base->size : 1);
  // printf("  push rax\n");
  printf("  #%04x SUB2\n", ty->base ? ty->base->size : 1);
}

static void gen_binary(Node *node) {
  // printf("  pop rdi\n");
  // printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
  case ND_ADD_EQ:
    // printf("  add rax, rdi\n");
    printf("  ADD2\n");
    break;
  case ND_PTR_ADD:
  case ND_PTR_ADD_EQ:
    // printf("  imul rdi, %d\n", node->ty->base->size);
    // printf("  add rax, rdi\n");
    printf("  #%04x MUL2 ADD2\n", node->ty->base->size);
    break;
  case ND_SUB:
  case ND_SUB_EQ:
    // printf("  sub rax, rdi\n");
    printf("  SUB2\n");
    break;
  case ND_PTR_SUB:
  case ND_PTR_SUB_EQ:
    // printf("  imul rdi, %d\n", node->ty->base->size);
    // printf("  sub rax, rdi\n");
    printf("  #%04x MUL2 SUB2\n", node->ty->base->size);
    break;
  case ND_PTR_DIFF:
    // printf("  sub rax, rdi\n");
    // printf("  cqo\n");
    // printf("  mov rdi, %d\n", node->lhs->ty->base->size);
    // printf("  idiv rdi\n");
    printf("  SUB2 #%04x DIV2\n", node->lhs->ty->base->size);

    break;
  case ND_MUL:
  case ND_MUL_EQ:
    // printf("  imul rax, rdi\n");
    printf("  MUL2\n");
    break;
  case ND_DIV:
  case ND_DIV_EQ:
    // printf("  cqo\n");
    // printf("  idiv rdi\n");
    need_sdiv_helper = true;
    printf("  sdiv\n");
    break;
  case ND_MOD:
  case ND_MOD_EQ:
    // printf("  cqo\n");
    // printf("  idiv rdi\n");
    need_sdiv_helper = true;
    printf("  OVR2 OVR2 sdiv MUL2 SUB2\n");
    break;
  case ND_BITAND:
  case ND_BITAND_EQ:
    // printf("  and rax, rdi\n");
    printf("  AND2\n");
    break;
  case ND_BITOR:
  case ND_BITOR_EQ:
    // printf("  or rax, rdi\n");
    printf("  ORA2\n");
    break;
  case ND_BITXOR:
  case ND_BITXOR_EQ:
    // printf("  xor rax, rdi\n");
    printf("  EOR2\n");
    break;
  case ND_SHL:
  case ND_SHL_EQ:
    // printf("  mov cl, dil\n");
    // printf("  shl rax, cl\n");
    printf("  NIP #40 SFT SFT2\n");
    break;
  case ND_SHR:
  case ND_SHR_EQ:
    // printf("  mov cl, dil\n");
    // printf("  sar rax, cl\n");
    need_ashr_helper = 1;
    printf("  ashr\n");
    break;
  case ND_EQ:
    // printf("  cmp rax, rdi\n");
    // printf("  sete al\n");
    // printf("  movzb rax, al\n");
    printf("  EQU2 #00 SWP\n");
    break;
  case ND_NE:
    // printf("  cmp rax, rdi\n");
    // printf("  setne al\n");
    // printf("  movzb rax, al\n");
    printf("  NEQ2 #00 SWP\n");
    break;
  case ND_LT:
    // printf("  cmp rax, rdi\n");
    // printf("  setl al\n");
    // printf("  movzb rax, al\n");
    need_slt_helper = true;
    printf("  slt\n");
    break;
  case ND_LE:
    // printf("  cmp rax, rdi\n");
    // printf("  setle al\n");
    // printf("  movzb rax, al\n");
    need_sle_helper = true;
    printf("  sle\n");
    break;
  }

  // printf("  push rax\n");
}

void args_backwards(Node *node) {
  if (node->next) {
    args_backwards(node->next);
  }
  gen(node);
}

// Generate code for a given node.
static void gen(Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    // if (node->val == (int)node->val) {
    //   printf("  push %ld\n", node->val);
    // } else {
    //   printf("  movabs rax, %ld\n", node->val);
    //   printf("  push rax\n");
    // }
    printf("  #%04x\n", (unsigned short)node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    // printf("  add rsp, 8\n");
    printf("  POP2\n");
    return;
  case ND_VAR:
    if (node->init)
      gen(node->init);
    gen_addr(node);
    if (node->ty->kind != TY_ARRAY)
      load(node->ty);
    return;
  case ND_MEMBER:
    gen_addr(node);
    if (node->ty->kind != TY_ARRAY)
      load(node->ty);
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    store(node->ty);
    return;
  case ND_TERNARY: {
    int seq = labelseq++;
    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    printf("  #0000 EQU2 ?L.else.%d\n", seq);
    gen(node->then);
    printf("  !L.end.%d\n", seq);
    printf("@L.else.%d\n", seq);
    gen(node->els);
    printf("@L.end.%d\n", seq);
    return;
  }
  case ND_PRE_INC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    printf("  DUP2\n");
    load(node->ty);
    inc(node->ty);
    store(node->ty);
    return;
  case ND_PRE_DEC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    printf("  DUP2\n");
    load(node->ty);
    dec(node->ty);
    store(node->ty);
    return;
  case ND_POST_INC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    printf("  DUP2\n");
    load(node->ty);
    printf("  DUP2\n");
    inc(node->ty);
    store_inner(node->ty);
    // dec(node->ty);
    return;
  case ND_POST_DEC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    printf("  DUP2\n");
    load(node->ty);
    printf("  DUP2\n");
    dec(node->ty);
    store_inner(node->ty);
    // inc(node->ty);
    return;
  case ND_ADD_EQ:
  case ND_PTR_ADD_EQ:
  case ND_SUB_EQ:
  case ND_PTR_SUB_EQ:
  case ND_MUL_EQ:
  case ND_DIV_EQ:
  case ND_SHL_EQ:
  case ND_SHR_EQ:
  case ND_BITAND_EQ:
  case ND_BITOR_EQ:
  case ND_BITXOR_EQ:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    printf("  DUP2\n");
    load(node->lhs->ty);
    gen(node->rhs);
    gen_binary(node);
    store(node->ty);
    return;
  case ND_COMMA:
    gen(node->lhs);
    gen(node->rhs);
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->ty->kind != TY_ARRAY)
      load(node->ty);
    return;
  case ND_NOT:
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  sete al\n");
    // printf("  movzb rax, al\n");
    // printf("  push rax\n");
    printf("  #0000 EQU2 #00 SWP\n");
    return;
  case ND_BITNOT:
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  not rax\n");
    // printf("  push rax\n");
    printf("  #ffff EOR2\n");
    return;
  case ND_LOGAND: {
    int seq = labelseq++;
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.false.%d\n", seq);
    printf("  #0000 EQU2 ?L.false.%d\n", seq);
    gen(node->rhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.false.%d\n", seq);
    // printf("  push 1\n");
    // printf("  jmp L.end.%d\n", seq);
    // printf("L.false.%d:\n", seq);
    // printf("  push 0\n");
    // printf("L.end.%d:\n", seq);
    printf("  #0000 EQU2 ?L.false.%d\n", seq);
    printf("  #0001 !L.end.%d\n", seq);
    printf("@L.false.%d\n", seq);
    printf("  #0000\n");
    printf("@L.end.%d\n", seq);
    return;
  }
  case ND_LOGOR: {
    int seq = labelseq++;
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.true.%d\n", seq);
    printf("  ORA ?L.true.%d\n", seq);
    gen(node->rhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.true.%d\n", seq);
    // printf("  push 0\n");
    // printf("  jmp L.end.%d\n", seq);
    // printf("L.true.%d:\n", seq);
    // printf("  push 1\n");
    // printf("L.end.%d:\n", seq);
    printf("  ORA ?L.true.%d\n", seq);
    printf("  #0000 !L.end.%d\n", seq);
    printf("@L.true.%d\n", seq);
    printf("  #0001\n");
    printf("@L.end.%d\n", seq);
    return;
  }
  case ND_IF: {
    int seq = labelseq++;
    if (node->els) {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.else.%d\n", seq);
      printf("  #0000 EQU2 ?L.else.%d\n", seq);
      gen(node->then);
      // printf("  jmp L.end.%d\n", seq);
      // printf("L.else.%d:\n", seq);
      printf("  !L.end.%d\n", seq);
      printf("@L.else.%d\n", seq);
      gen(node->els);
      // printf("L.end.%d:\n", seq);
      printf("@L.end.%d\n", seq);
    } else {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.end.%d\n", seq);
      printf("  #0000 EQU2 ?L.end.%d\n", seq);
      gen(node->then);
      // printf("L.end.%d:\n", seq);
      printf("@L.end.%d\n", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    int brk = brkseq;
    int cont = contseq;
    brkseq = contseq = seq;

    // printf("L.continue.%d:\n", seq);
    printf("@L.continue.%d\n", seq);

    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.break.%d\n", seq);
    printf("  #0000 EQU2 ?L.break.%d\n", seq);

    gen(node->then);
    // printf("  jmp L.continue.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    printf("  !L.continue.%d\n", seq);
    printf("@L.break.%d\n", seq);

    brkseq = brk;
    contseq = cont;
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    int brk = brkseq;
    int cont = contseq;
    brkseq = contseq = seq;

    if (node->init)
      gen(node->init);
    // printf("L.begin.%d:\n", seq);
    printf("@L.begin.%d\n", seq);
    if (node->cond) {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.break.%d\n", seq);
      printf("  #0000 EQU2 ?L.break.%d\n", seq);
    }
    gen(node->then);
    // printf("L.continue.%d:\n", seq);
    printf("@L.continue.%d\n", seq);
    if (node->inc)
      gen(node->inc);
    // printf("  jmp L.begin.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    printf("  !L.begin.%d\n", seq);
    printf("@L.break.%d\n", seq);

    brkseq = brk;
    contseq = cont;
    return;
  }
  case ND_DO: {
    int seq = labelseq++;
    int brk = brkseq;
    int cont = contseq;
    brkseq = contseq = seq;

    // printf("L.begin.%d:\n", seq);
    printf("@L.begin.%d\n", seq);
    gen(node->then);
    // printf("L.continue.%d:\n", seq);
    printf("@L.continue.%d\n", seq);
    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.begin.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    printf("  #0000 NEQ2 ?L.begin.%d\n", seq);
    printf("@L.break.%d\n", seq);

    brkseq = brk;
    contseq = cont;
    return;
  }
  case ND_SWITCH: {
    int seq = labelseq++;
    int brk = brkseq;
    brkseq = seq;
    node->case_label = seq;

    gen(node->cond);
    // printf("  pop rax\n");

    for (Node *n = node->case_next; n; n = n->case_next) {
      n->case_label = labelseq++;
      n->case_end_label = seq;
      // printf("  cmp rax, %ld\n", n->val);
      // printf("  je L.case.%d\n", n->case_label);
      printf("  DUP2 #%04x EQU2 ?L.case.%d\n", (unsigned short)n->val,
             n->case_label);
    }

    if (node->default_case) {
      int i = labelseq++;
      node->default_case->case_end_label = seq;
      node->default_case->case_label = i;
      printf("  !L.case.%d\n", i);
    }

    printf("  !L.break.%d\n", seq);
    gen(node->then);
    printf("@L.break.%d\n", seq);
    printf("  POP2\n");

    brkseq = brk;
    return;
  }
  case ND_CASE:
    printf("@L.case.%d\n", node->case_label);
    gen(node->lhs);
    return;
  case ND_BLOCK:
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen(n);
    return;
  case ND_BREAK:
    if (brkseq == 0)
      error_tok(node->tok, "stray break");
    // printf("  jmp L.break.%d\n", brkseq);
    printf("  !L.break.%d\n", brkseq);
    return;
  case ND_CONTINUE:
    if (contseq == 0)
      error_tok(node->tok, "stray continue");
    // printf("  jmp L.continue.%d\n", contseq);
    printf("  !L.continue.%d\n", contseq);
    return;
  case ND_GOTO:
    // printf("  jmp L.label.%s.%s\n", funcname, node->label_name);
    printf("  !L.label.%s.%s\n", funcname, node->label_name);
    return;
  case ND_LABEL:
    // printf("L.label.%s.%s:\n", funcname, node->label_name);
    printf("@L.label.%s.%s\n", funcname, node->label_name);
    gen(node->lhs);
    return;
  case ND_FUNCALL: {
    if (!strcmp(node->funcname, "deo")) {
      gen(node->args);
      printf("  NIP\n");
      gen(node->args->next);
      printf("  NIP DEOk\n"); // Will be followed by POP2
      return;
    }
    if (!strcmp(node->funcname, "deo2")) {
      gen(node->args);
      gen(node->args->next);
      printf("  NIP DEO2k POP\n"); // Will be followed by POP2
      return;
    }
    if (!strcmp(node->funcname, "dei")) {
      gen(node->args);
      printf("  NIP DEI #00 SWP\n");
      return;
    }
    if (!strcmp(node->funcname, "dei2")) {
      gen(node->args);
      printf("  NIP DEI2\n");
      return;
    }
    if (!strcmp(node->funcname, "__builtin_va_start")) {
      // printf("  pop rax\n");
      // printf("  mov edi, dword ptr [rbp-8]\n");
      // printf("  mov dword ptr [rax], 0\n");
      // printf("  mov dword ptr [rax+4], 0\n");
      // printf("  mov qword ptr [rax+8], rdi\n");
      // printf("  mov qword ptr [rax+16], 0\n");
      error("unsupported __builtin_va_start");
      return;
    }

    // Function arguments are passed via the uxn working stack.
    if (node->args)
      args_backwards(node->args);

    // for (int i = nargs - 1; i >= 0; i--)
    //   printf("  pop %s\n", argreg8[i]);

    // We need to align RSP to a 16 byte boundary before
    // calling a function because it is an ABI requirement.
    // RAX is set to 0 for variadic function.
    // int seq = labelseq++;
    // printf("  mov rax, rsp\n");
    // printf("  and rax, 15\n");
    // printf("  jnz L.call.%d\n", seq);
    // printf("  mov rax, 0\n");
    // printf("  call %s\n", node->funcname);
    // printf("  jmp L.end.%d\n", seq);
    // printf("L.call.%d:\n", seq);
    // printf("  sub rsp, 8\n");
    // printf("  mov rax, 0\n");
    // printf("  call %s\n", node->funcname);
    // printf("  add rsp, 8\n");
    // printf("L.end.%d:\n", seq);
    // if (node->ty->kind == TY_BOOL)
    //   printf("  movzb rax, al\n");
    // printf("  push rax\n");
    printf("  %s_\n", node->funcname);
    return;
  }
  case ND_RETURN:
    if (node->lhs) {
      gen(node->lhs);
      // printf("  pop rax\n");
    } else {
      printf("  #0000\n"); // dummy return value
    }
    printf("  !L.return.%s\n", funcname);
    return;
  case ND_CAST:
    gen(node->lhs);
    truncate(node->ty);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);
  gen_binary(node);
}

static void emit_string_literal(Initializer *init) {
  putchar(' ');
  int n = 0;
  for (; init; init = init->next) {
    char c = init->val;
    if (c >= '!' && c <= '~') {
      if (n++ == 0)
        printf(" \"");
      putchar(c);
      // Avoid running into uxnasm's token length limit.
      if (n >= 60)
        n = 0;
    } else {
      printf(" %02x", c);
      n = 0;
    }
  }
  putchar('\n');
}

static void emit_data(Program *prog) {
  // for (VarList *vl = prog->globals; vl; vl = vl->next)
  //   if (!vl->var->is_static)
  //     printf(".global %s\n", vl->var->name);

  printf("( bss )\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (var->initializer)
      continue;

    // printf("( align %d )\n", var->ty->align);
    // Name is suffixed with _ so that uxnasm won't complain if it happens to be
    // hexadecimal.
    printf("@%s_ $%x\n", var->name, var->ty->size);
  }

  printf("( data )\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (!var->initializer)
      continue;

    // printf(".align %d\n", var->ty->align);
    printf("@%s_\n", var->name);

    if (var->is_string_literal) {
      emit_string_literal(var->initializer);
      continue;
    }

    int column = 0;
    for (Initializer *init = var->initializer; init; init = init->next) {
      if (column == 0)
        putchar(' ');
      if (init->label) {
        if (init->addend) {
          // uxntal sadly has no way to express an offset to an addressing rune
          error("unsupported initializer for var \"%s\": can't handle non-zero "
                "addend (%+ld) with label (\"%s\")",
                var->name, init->addend, init->label);
        }
        printf(" =%s_", init->label);
        column += 2;
      } else if (init->sz == 1) {
        printf(" %02x", (unsigned char)init->val);
        column += 1;
      } else if (init->sz == 2) {
        printf(" %04x", (unsigned short)init->val);
        column += 2;
      } else {
        error("unsupported initializer size");
      }
      if (column >= 16) {
        putchar('\n');
        column = 0;
      }
    }
    putchar('\n');
  }
}

static void load_arg(Var *var /*, int idx*/) {
  printf("  STH2rk\n");
  emit_add(var->offset);
  printf(var->ty->size == 1 ? "STA POP\n" : "STA2\n");

  // int sz = var->ty->size;
  // if (sz == 1) {
  //   printf("  mov [rbp-%d], %s\n", var->offset, argreg1[idx]);
  // } else if (sz == 2) {
  //   printf("  mov [rbp-%d], %s\n", var->offset, argreg2[idx]);
  // } else if (sz == 4) {
  //   printf("  mov [rbp-%d], %s\n", var->offset, argreg4[idx]);
  // } else {
  //   assert(sz == 8);
  //   printf("  mov [rbp-%d], %s\n", var->offset, argreg8[idx]);
  // }
}

static void emit_text(Program *prog) {
  printf("( text )\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    // if (!fn->is_static)
    //   printf(".global %s\n", fn->name);
    // Name is suffixed with _ so that uxnasm won't complain if it happens to be
    // hexadecimal.
    printf("@%s_\n", fn->name);
    funcname = fn->name;

    // Prologue
    // printf("  push rbp\n");
    // printf("  mov rbp, rsp\n");
    // printf("  sub rsp, %d\n", fn->stack_size);

    // Copy the frame pointer from the "frame" below.
    printf("  OVR2r\n");
    if (fn->stack_size != 0) {
      printf("  LIT2r %04x SUB2r\n", fn->stack_size);
    }

    // Save arg registers if function is variadic
    if (fn->has_varargs) {
      error("unsupported varargs");
      // int n = 0;
      // for (VarList *vl = fn->params; vl; vl = vl->next)
      //   n++;

      // printf("mov dword ptr [rbp-8], %d\n", n * 8);
      // printf("mov [rbp-16], r9\n");
      // printf("mov [rbp-24], r8\n");
      // printf("mov [rbp-32], rcx\n");
      // printf("mov [rbp-40], rdx\n");
      // printf("mov [rbp-48], rsi\n");
      // printf("mov [rbp-56], rdi\n");
    }

    // Push arguments to the in-memory stack from the uxn working stack
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next)
      load_arg(vl->var /*, i++*/);

    // Emit code
    for (Node *node = fn->node; node; node = node->next)
      gen(node);

    // Epilogue
    printf("  #0000\n"); // dummy return value
    printf("@L.return.%s\n", funcname);
    // printf("  mov rsp, rbp\n");
    // printf("  pop rbp\n");
    // printf("  ret\n");

    // Pop the frame pointer, then return.
    printf("  POP2r JMP2r\n");
  }

  if (need_sext_helper) {
    // 8-bit to 16-bit sign extension
    printf("@sext\n");
    printf("  #80 ANDk EQU #ff MUL SWP JMP2r\n");
  }
  if (need_ashr_helper) {
    // 16-bit arithmetic right shift (uxn's SFT does logical right shift)
    // If the sign bit was unset, this is ((uint16_t)a >> b)
    // If the sign bit was set, this is ~((uint16_t)(~a) >> b)
    // The double NOT has the effect of making the zeroes shifted in become ones
    // to match the sign bit
    printf("@ashr\n");
    // Swap value to be shifted with shift amount
    printf("  SWP2\n");
    // Check sign bit and convert it into an XOR mask
    printf("  #8000 AND2k EQU2 #ff MUL DUP\n");
    // XOR before shifting
    printf("  DUP2 ROT2 EOR2 ROT2\n");
    // XOR, shift, XOR again
    // Convert shift amount to SFT format and do shift
    printf("  NIP #0f AND SFT2\n");
    // XOR again
    printf("  EOR2\n");
    // Return
    printf("  JMP2r\n");
  }
  if (need_slt_helper) {
    // Signed less-than
    // uxn's LTH is unsigned, so we flip the sign bits to get signed comparison
    // The masking requires some stack juggling, so we use GTH to save a SWP.
    printf("@slt\n");
    printf("  #8000 EOR2 SWP2 #8000 EOR2 GTH2 #00 SWP JMP2r\n");
  }
  if (need_sle_helper) {
    // Signed less-than-or-equal-to
    // Same deal as with less-than, but using (a <= b) == !(a > b)
    printf("@sle\n");
    printf("  #8000 EOR2 SWP2 #8000 EOR2 LTH2 #00 SWP #01 EOR JMP2r\n");
  }
  if (need_sdiv_helper) {
    // Signed division (uxn's DIV is unsigned)
    printf("@sdiv\n");
    // Get the sign bits of the two inputs and combine them into a single byte
    printf("  OVR2 #4f SFT2 OVR2 #0f SFT2 ORA2 NIP\n");
    // Branch accordingly
    printf("  DUP #00 EQU ?&pospos\n"); // most common case first
    printf("  DUP #01 EQU ?&posneg\n");
    printf("  #10 EQU ?&negpos\n");
    // (-a / -b)
    printf("  #0000 ROT2 SUB2 SWP2 #0000 SWP2 SUB2 DIV2 JMP2r\n");
    // (a / b)
    printf("  &pospos POP DIV2 JMP2r\n");
    // -(a / -b)
    printf("  &posneg POP #0000 SWP2 SUB2 DIV2 #0000 SWP2 SUB2 JMP2r\n");
    // -(-a / b)
    printf("  &negpos SWP2 #0000 SWP2 SUB2 SWP2 DIV2 #0000 SWP2 SUB2 JMP2r\n");
  }
}

typedef struct {
  char *name;
  unsigned char port;
} Device;

static Device devices[] = {
    {"console", 0x10}, {"screen", 0x20}, {"audio1", 0x30},     {"audio2", 0x40},
    {"audio3", 0x50},  {"audio4", 0x60}, {"controller", 0x80}, {"mouse", 0x90},
};

void codegen(Program *prog) {
  int i;
  bool need_device_hook[sizeof(devices) / sizeof(Device)] = {false};

  printf("|0100\n");
  for (Function *fn = prog->fns; fn; fn = fn->next) {
    for (i = 0; i < sizeof(devices) / sizeof(Device); i++) {
      if (*devices[i].name && !strncmp(fn->name, "on_", 3) &&
          !strcmp(fn->name + 3, devices[i].name)) {
        printf("  ;L.%s.hook #%02x DEO2\n", devices[i].name, devices[i].port);
        need_device_hook[i] = true;
      }
    }
  }
  printf("  LIT2r 0000 main_ POP2r BRK\n");
  for (i = 0; i < sizeof(devices) / sizeof(Device); i++) {
    if (need_device_hook[i]) {
      printf("  @L.%s.hook LIT2r 0000 on_%s_ POP2 POP2r BRK\n", devices[i].name,
             devices[i].name);
    }
  }
  emit_data(prog);
  emit_text(prog);
}
