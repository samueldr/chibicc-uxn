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
static bool need_sdiv_helper;

static void gen(Node *node);

static void emit_add(int n) {
  if (n != 0) {
    lit2(n);
    op(ADD2);
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
      op(STH2kr);
      emit_add(var->offset);
    } else {
      semi("%s_", var->name);
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
  default:
    error_tok(node->tok, "not an lvalue");
    return;
  }
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
    op(LDA);
    jsi("sext");
  } else {
    op(LDA2);
  }
}

// Expects "newval addr" on the stack and leaves "newval".
static void store(Type *ty) {
  if (ty->size == 1) {
    op(STA | flag_k);
    op(POP2);
  } else {
    op(STA2 | flag_k);
    op(POP2);
  }
}

static void fix_bool(Type *ty) {
  if (ty->kind == TY_BOOL) {
    lit2(0);
    op(NEQ2);
    lit(0);
    op(SWP);
  }
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
    fix_bool(ty);
  } else if (ty->size == 1) {
    need_sext_helper = 1;
    op(NIP);
    jsi("sext");
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
  lit2(ty->base ? ty->base->size : 1);
  op(SUB2);
}

static void gen_binary(Node *node) {
  // printf("  pop rdi\n");
  // printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
  case ND_ADD_EQ:
    // printf("  add rax, rdi\n");
    op(ADD2);
    break;
  case ND_PTR_ADD:
  case ND_PTR_ADD_EQ:
    // printf("  imul rdi, %d\n", node->ty->base->size);
    // printf("  add rax, rdi\n");
    lit2(node->ty->base->size);
    op(MUL2);
    op(ADD2);
    break;
  case ND_SUB:
  case ND_SUB_EQ:
    // printf("  sub rax, rdi\n");
    op(SUB2);
    break;
  case ND_PTR_SUB:
  case ND_PTR_SUB_EQ:
    // printf("  imul rdi, %d\n", node->ty->base->size);
    // printf("  sub rax, rdi\n");
    lit2(node->ty->base->size);
    op(MUL2);
    op(SUB2);
    break;
  case ND_PTR_DIFF:
    // printf("  sub rax, rdi\n");
    // printf("  cqo\n");
    // printf("  mov rdi, %d\n", node->lhs->ty->base->size);
    // printf("  idiv rdi\n");
    op(SUB2);
    lit2(node->lhs->ty->base->size);
    op(DIV2);
    break;
  case ND_MUL:
  case ND_MUL_EQ:
    // printf("  imul rax, rdi\n");
    op(MUL2);
    break;
  case ND_DIV:
  case ND_DIV_EQ:
    // printf("  cqo\n");
    // printf("  idiv rdi\n");
    need_sdiv_helper = true;
    jsi("sdiv");
    break;
  case ND_MOD:
  case ND_MOD_EQ:
    // printf("  cqo\n");
    // printf("  idiv rdi\n");
    need_sdiv_helper = true;
    op(OVR2);
    op(OVR2);
    jsi("sdiv");
    op(MUL2);
    op(SUB2);
    break;
  case ND_BITAND:
  case ND_BITAND_EQ:
    // printf("  and rax, rdi\n");
    op(AND2);
    break;
  case ND_BITOR:
  case ND_BITOR_EQ:
    // printf("  or rax, rdi\n");
    op(ORA2);
    break;
  case ND_BITXOR:
  case ND_BITXOR_EQ:
    // printf("  xor rax, rdi\n");
    op(EOR2);
    break;
  case ND_SHL:
  case ND_SHL_EQ:
    // printf("  mov cl, dil\n");
    // printf("  shl rax, cl\n");
    op(NIP);
    lit(0x40);
    op(SFT);
    op(SFT2);
    break;
  case ND_SHR:
  case ND_SHR_EQ:
    // printf("  mov cl, dil\n");
    // printf("  sar rax, cl\n");
    need_ashr_helper = 1;
    jsi("ashr");
    break;
  case ND_EQ:
    // printf("  cmp rax, rdi\n");
    // printf("  sete al\n");
    // printf("  movzb rax, al\n");
    op(EQU2);
    lit(0);
    op(SWP);
    break;
  case ND_NE:
    // printf("  cmp rax, rdi\n");
    // printf("  setne al\n");
    // printf("  movzb rax, al\n");
    op(NEQ2);
    lit(0);
    op(SWP);
    break;
  default:
    error_tok(node->tok, "not a binary operation");
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
    lit2(node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    // printf("  add rsp, 8\n");
    op(POP2);
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
    gen(node->rhs);
    fix_bool(node->ty);
    gen_lval(node->lhs);
    store(node->ty);
    return;
  case ND_TERNARY: {
    int seq = labelseq++;
    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    lit2(0);
    op(EQU2);
    jci("L.else.%d", seq);
    gen(node->then);
    jmi("L.end.%d", seq);
    at("L.else.%d", seq);
    gen(node->els);
    at("L.end.%d", seq);
    return;
  }
  case ND_PRE_INC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    op(DUP2);
    load(node->ty);
    inc(node->ty);
    fix_bool(node->ty);
    op(SWP2);
    store(node->ty);
    return;
  case ND_PRE_DEC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    op(DUP2);
    load(node->ty);
    dec(node->ty);
    fix_bool(node->ty);
    op(SWP2);
    store(node->ty);
    return;
  case ND_POST_INC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    op(DUP2);
    load(node->ty);
    op(DUP2);
    inc(node->ty);
    fix_bool(node->ty);
    op(ROT2);
    store(node->ty);
    op(POP2);
    // dec(node->ty);
    return;
  case ND_POST_DEC:
    gen_lval(node->lhs);
    // printf("  push [rsp]\n");
    op(DUP2);
    load(node->ty);
    op(DUP2);
    dec(node->ty);
    fix_bool(node->ty);
    op(ROT2);
    store(node->ty);
    op(POP2);
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
    op(DUP2);
    load(node->lhs->ty);
    gen(node->rhs);
    gen_binary(node);
    fix_bool(node->ty);
    op(SWP2);
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
    lit2(0);
    op(EQU2);
    lit(0);
    op(SWP);
    return;
  case ND_BITNOT:
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  not rax\n");
    // printf("  push rax\n");
    lit2(0xffff);
    op(EOR2);
    return;
  case ND_LOGAND: {
    int seq = labelseq++;
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.false.%d\n", seq);
    lit2(0);
    op(EQU2);
    jci("L.false.%d", seq);
    gen(node->rhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.false.%d\n", seq);
    // printf("  push 1\n");
    // printf("  jmp L.end.%d\n", seq);
    // printf("L.false.%d:\n", seq);
    // printf("  push 0\n");
    // printf("L.end.%d:\n", seq);
    lit2(0);
    op(EQU2);
    jci("L.false.%d", seq);
    lit2(1);
    jmi("L.end.%d", seq);
    at("L.false.%d", seq);
    lit2(0);
    at("L.end.%d", seq);
    return;
  }
  case ND_LOGOR: {
    int seq = labelseq++;
    gen(node->lhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.true.%d\n", seq);
    op(ORA);
    jci("L.true.%d", seq);
    gen(node->rhs);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.true.%d\n", seq);
    // printf("  push 0\n");
    // printf("  jmp L.end.%d\n", seq);
    // printf("L.true.%d:\n", seq);
    // printf("  push 1\n");
    // printf("L.end.%d:\n", seq);
    op(ORA);
    jci("L.true.%d", seq);
    lit2(0);
    jmi("L.end.%d", seq);
    at("L.true.%d", seq);
    lit2(1);
    at("L.end.%d", seq);
    return;
  }
  case ND_IF: {
    int seq = labelseq++;
    if (node->els) {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.else.%d\n", seq);
      lit2(0);
      op(EQU2);
      jci("L.else.%d", seq);
      gen(node->then);
      // printf("  jmp L.end.%d\n", seq);
      // printf("L.else.%d:\n", seq);
      jmi("L.end.%d", seq);
      at("L.else.%d", seq);
      gen(node->els);
      // printf("L.end.%d:\n", seq);
      at("L.end.%d", seq);
    } else {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.end.%d\n", seq);
      lit2(0);
      op(EQU2);
      jci("L.end.%d", seq);
      gen(node->then);
      // printf("L.end.%d:\n", seq);
      at("L.end.%d", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    int brk = brkseq;
    int cont = contseq;
    brkseq = contseq = seq;

    // printf("L.continue.%d:\n", seq);
    at("L.continue.%d", seq);

    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  je  L.break.%d\n", seq);
    lit2(0);
    op(EQU2);
    jci("L.break.%d", seq);

    gen(node->then);
    // printf("  jmp L.continue.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    jmi("L.continue.%d", seq);
    at("L.break.%d", seq);

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
    at("L.begin.%d", seq);
    if (node->cond) {
      gen(node->cond);
      // printf("  pop rax\n");
      // printf("  cmp rax, 0\n");
      // printf("  je  L.break.%d\n", seq);
      lit2(0);
      op(EQU2);
      jci("L.break.%d", seq);
    }
    gen(node->then);
    // printf("L.continue.%d:\n", seq);
    at("L.continue.%d", seq);
    if (node->inc)
      gen(node->inc);
    // printf("  jmp L.begin.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    jmi("L.begin.%d", seq);
    at("L.break.%d", seq);

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
    at("L.begin.%d", seq);
    gen(node->then);
    // printf("L.continue.%d:\n", seq);
    at("L.continue.%d", seq);
    gen(node->cond);
    // printf("  pop rax\n");
    // printf("  cmp rax, 0\n");
    // printf("  jne L.begin.%d\n", seq);
    // printf("L.break.%d:\n", seq);
    lit2(0);
    op(NEQ2);
    jci("L.begin.%d", seq);
    at("L.break.%d", seq);

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
      op(DUP2);
      lit2(n->val);
      op(EQU2);
      jci("L.case.%d", n->case_label);
    }

    if (node->default_case) {
      int i = labelseq++;
      node->default_case->case_end_label = seq;
      node->default_case->case_label = i;
      jmi("L.case.%d", i);
    }

    jmi("L.break.%d", seq);
    gen(node->then);
    at("L.break.%d", seq);
    op(POP2);

    brkseq = brk;
    return;
  }
  case ND_CASE:
    at("L.case.%d", node->case_label);
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
    jmi("L.break.%d", brkseq);
    return;
  case ND_CONTINUE:
    if (contseq == 0)
      error_tok(node->tok, "stray continue");
    // printf("  jmp L.continue.%d\n", contseq);
    jmi("L.continue.%d", contseq);
    return;
  case ND_GOTO:
    // printf("  jmp L.label.%s.%s\n", funcname, node->label_name);
    jmi("L.label.%s.%s", funcname, node->label_name);
    return;
  case ND_LABEL:
    // printf("L.label.%s.%s:\n", funcname, node->label_name);
    at("L.label.%s.%s", funcname, node->label_name);
    gen(node->lhs);
    return;
  case ND_FUNCALL: {
    if (!strcmp(node->funcname, "deo")) {
      gen(node->args);
      op(NIP);
      gen(node->args->next);
      op(NIP);
      op(DEO);
      lit2(0); // Will be followed by POP2
      return;
    }
    if (!strcmp(node->funcname, "deo2")) {
      gen(node->args);
      gen(node->args->next);
      op(NIP);
      op(DEO2);
      lit2(0); // Will be followed by POP2
      return;
    }
    if (!strcmp(node->funcname, "dei")) {
      gen(node->args);
      op(NIP);
      op(DEI);
      lit(0);
      op(SWP);
      return;
    }
    if (!strcmp(node->funcname, "dei2")) {
      gen(node->args);
      op(NIP);
      op(DEI2);
      return;
    }
    if (!strcmp(node->funcname, "exit")) {
      gen(node->args);
      op(NIP);
      lit(0x80);
      op(ORA);
      lit(0x0f);
      op(DEO);
      op(BRK);
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
    jsi("%s_", node->funcname);
    return;
  }
  case ND_RETURN:
    if (node->lhs) {
      gen(node->lhs);
      // printf("  pop rax\n");
    } else {
      lit2(0); // dummy return value
    }
    jmi("L.return.%s", funcname);
    return;
  case ND_CAST:
    gen(node->lhs);
    truncate(node->ty);
    return;
  case ND_LT:
    gen(node->lhs);
    lit2(0x8000);
    op(EOR2);
    gen(node->rhs);
    lit2(0x8000);
    op(EOR2);
    op(LTH2);
    lit(0);
    op(SWP);
    return;
  case ND_LE:
    gen(node->lhs);
    lit2(0x8000);
    op(EOR2);
    gen(node->rhs);
    lit2(0x8000);
    op(EOR2);
    op(GTH2);
    lit(0);
    op(SWP);
    lit(1);
    op(EOR);
    return;

  default: // Binary operations
    gen(node->lhs);
    gen(node->rhs);
    gen_binary(node);
  }
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
  op(STH2kr);
  emit_add(var->offset);
  if (var->ty->size == 1) {
    op(STA);
    op(POP);
  } else {
    op(STA2);
  }

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
    at("%s_", fn->name);
    funcname = fn->name;

    // Prologue
    // printf("  push rbp\n");
    // printf("  mov rbp, rsp\n");
    // printf("  sub rsp, %d\n", fn->stack_size);

    // Copy the frame pointer from the "frame" below.
    op(OVR2 | flag_r);
    if (fn->stack_size != 0) {
      lit2r(fn->stack_size);
      op(SUB2 | flag_r);
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
    lit2(0); // dummy return value
    at("L.return.%s", funcname);
    // printf("  mov rsp, rbp\n");
    // printf("  pop rbp\n");
    // printf("  ret\n");

    // Pop the frame pointer, then return.
    truncate(fn->ty);
    op(POP2r);
    op(JMP2r);
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

void codegen(Program *prog, bool do_opt) {
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

  Instruction out = {};
  emit_head = &out;
  emit_text(prog);
  if (do_opt)
    optimize(&out);
  output(&out);
}
