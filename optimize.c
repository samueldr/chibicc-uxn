#include "chibi.h"

char *opcode_names[0x20] = {
  "BRK", "INC", "POP", "NIP", "SWP", "ROT", "DUP", "OVR",
  "EQU", "NEQ", "GTH", "LTH", "JMP", "JCN", "JSR", "STH",
  "LDZ", "STZ", "LDR", "STR", "LDA", "STA", "DEI", "DEO",
  "ADD", "SUB", "MUL", "DIV", "AND", "ORA", "EOR", "SFT",
};

Instruction *emit_head;

void emit(Opcode opcode, int literal, char* label) {
  emit_head->opcode = opcode;
  emit_head->literal = literal;
  if (label)
    emit_head->label = strdup(label);
  emit_head->next = calloc(1, sizeof(Instruction));
  emit_head = emit_head->next;
}

char buf[64];
void op(Opcode o) { emit(o, 0, ""); }
void jci(char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(buf, fmt, va); va_end(va); emit(JCI, 0, buf); }
void jmi(char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(buf, fmt, va); va_end(va); emit(JMI, 0, buf); }
void jsi(char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(buf, fmt, va); va_end(va); emit(JSI, 0, buf); }
void lit(unsigned char n) { emit(LIT, n, ""); }
void lit2(unsigned short n) { emit(LIT2, n, ""); }
void lit2r(unsigned short n) { emit(LIT2r, n, ""); }
void at(char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(buf, fmt, va); va_end(va); emit(AT, 0, buf); }
void semi(char* fmt, ...) { va_list va; va_start(va, fmt); vsprintf(buf, fmt, va); va_end(va); emit(SEMI, 0, buf); }
void bar(unsigned short n) { emit(BAR, n, ""); }

#define ConstantFolding(o, x) \
    if (prog->opcode == LIT2 && prog->next->opcode == LIT2 && prog->next->next->opcode == (o | flag_2)) { \
      prog->literal = prog->literal x prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    } \
    if (prog->opcode == LIT && prog->next->opcode == LIT && prog->next->next->opcode == o) { \
      prog->literal = (unsigned char)prog->literal x (unsigned char)prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    }
#define ConstantFoldingComparison(o, x) \
    if (prog->opcode == LIT2 && prog->next->opcode == LIT2 && prog->next->next->opcode == (o | flag_2)) { \
      prog->opcode = LIT; /* always one-byte result */ \
      prog->literal = prog->literal x prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    } \
    if (prog->opcode == LIT && prog->next->opcode == LIT && prog->next->next->opcode == o) { \
      prog->literal = (unsigned char)prog->literal x (unsigned char)prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    }

static bool optimize_pass(Instruction* prog, int stage) {
  bool changed = false;
  while (prog->opcode) {
    if (prog->next && prog->next->next) {
      ConstantFolding(ADD, +);
      ConstantFolding(SUB, -);
      ConstantFolding(MUL, *);
      ConstantFolding(DIV, /);
      ConstantFolding(AND, &);
      ConstantFolding(ORA, |);
      ConstantFolding(EOR, ^);
      ConstantFoldingComparison(EQU, ==);
      ConstantFoldingComparison(NEQ, !=);
      ConstantFoldingComparison(GTH, >);
      ConstantFoldingComparison(LTH, <);
    }

    // Fold INC2
    if (prog->opcode == LIT2 && prog->next && prog->next->opcode == INC2) {
      ++prog->literal;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // #0001 ADD2 -> INC2
    if (prog->opcode == LIT2 && prog->literal == 1 && prog->next && prog->next->opcode == ADD2) {
      prog->opcode = INC2;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // #0000 +-|^ -> nothing
    if (prog->opcode == LIT2 && prog->literal == 0 && prog->next && (prog->next->opcode == ADD2 || prog->next->opcode == SUB2 || prog->next->opcode == ORA2 || prog->next->opcode == EOR2)) {
      memcpy(prog, prog->next->next, sizeof(Instruction));
      changed = true;
      continue;
    }

    // #xxxx POP2 cancels out
    if (prog->opcode == LIT2 && prog->next && prog->next->opcode == POP2) {
      memcpy(prog, prog->next->next, sizeof(Instruction));
      changed = true;
      continue;
    }

    // sext NIP cancels out
    if (prog->opcode == JSI && !strcmp("sext", prog->label) && prog->next && prog->next->opcode == NIP) {
      memcpy(prog, prog->next->next, sizeof(Instruction));
      changed = true;
      continue;
    }

    // #0001 muldiv -> nothing
    if (prog->opcode == LIT2 && prog->literal == 1 && prog->next && (prog->next->opcode == MUL2 || prog->next->opcode == DIV2)) {
      memcpy(prog, prog->next->next, sizeof(Instruction));
      changed = true;
      continue;
    }

    // pow2 muldiv -> SFT2
    if (prog->opcode == LIT2 && prog->literal > 1 && (prog->literal & (prog->literal - 1)) == 0 && prog->next && (prog->next->opcode == MUL2 || prog->next->opcode == DIV2)) {
      prog->opcode = LIT;
      int log2 = 0;
      while (prog->literal >>= 1) log2++;
      prog->literal = log2 << (prog->next->opcode == MUL2 ? 4 : 0);
      prog->next->opcode = SFT2;
      changed = true;
      continue;
    }

    // Shrink literals
    if (prog->opcode == LIT2 && prog->next && prog->next->opcode == NIP) {
      prog->opcode = LIT;
      prog->literal &= 0xff;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // Combine literals
    if (stage >= 1 && prog->opcode == LIT && prog->next && prog->next->opcode == LIT) {
      prog->opcode = LIT2;
      prog->literal = prog->literal << 8 | prog->next->literal;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // Associative addition: x ADD y ADD = (x+y) ADD
    if (prog->opcode == LIT2
        && prog->next && prog->next->opcode == ADD2
        && prog->next->next && prog->next->next->opcode == LIT2
        && prog->next->next->next && prog->next->next->next->opcode == ADD2) {
      prog->literal += prog->next->next->literal;
      prog->next->next = prog->next->next->next->next;
      changed = true;
      continue;
    }

    // A common sequence: STA2k POP2 POP2 -> STA2
    if (prog->opcode == STA2k
        && prog->next && prog->next->opcode == POP2
        && prog->next->next && prog->next->next->opcode == POP2) {
      prog->opcode = STA2;
      prog->next = prog->next->next->next;
      changed = true;
      continue;
    }

    // A common sequence: STAk POP2 POP2 -> STA POP
    if (prog->opcode == (STA | flag_k)
        && prog->next && prog->next->opcode == POP2
        && prog->next->next && prog->next->next->opcode == POP2) {
      prog->opcode = STA;
      prog->next->opcode = POP;
      prog->next->next = prog->next->next->next;
      changed = true;
      continue;
    }

    // DUP2 (INC2|LDA2|DUP2) -> +k
    if (prog->opcode == DUP2 && prog->next && (prog->next->opcode == INC2 || prog->next->opcode == LDA2 || prog->next->opcode == DUP2)) {
      prog->opcode = (prog->next->opcode | flag_k);
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // A common sequence: STH2kr #xxxx ADD2 #yyyy SWP2 -> #yyyy STH2kr #xxxx ADD2
    if (prog->opcode == STH2kr
        && prog->next && prog->next->opcode == LIT2
        && prog->next->next && prog->next->next->opcode == ADD2
        && prog->next->next->next && prog->next->next->next->opcode == LIT2
        && prog->next->next->next->next && prog->next->next->next->next->opcode == SWP2) {
      int x = prog->next->literal;
      prog->opcode = LIT2;
      prog->literal = prog->next->next->next->literal;
      prog->next->opcode = STH2kr;
      prog->next->next->opcode = LIT2;
      prog->next->next->literal = x;
      prog->next->next->next->opcode = ADD2;
      prog->next->next->next->next = prog->next->next->next->next->next;
      changed = true;
      continue;
    }

    // EQU2 #00 SWP #0000 EQU2 -> NEQ2
    if (prog->opcode == EQU2
        && prog->next && prog->next->opcode == LIT && prog->next->literal == 0
        && prog->next->next && prog->next->next->opcode == SWP
        && prog->next->next->next && prog->next->next->next->opcode == LIT2 && prog->next->literal == 0
        && prog->next->next->next->next && prog->next->next->next->next->opcode == EQU2) {
      prog->opcode = NEQ2;
      prog->next = prog->next->next->next->next->next;
    }

    // #0000 NEQ2 ? -> ORA ?
    if (prog->opcode == LIT2 && prog->literal == 0
        && prog->next && prog->next->opcode == NEQ2
        && prog->next->next && prog->next->next->opcode == JCI) {
      prog->opcode = ORA;
      prog->next = prog->next->next;
    }

    prog = prog->next;
  }
  return changed;
}

void optimize(Instruction* prog) {
  while (optimize_pass(prog, 0));
  while (optimize_pass(prog, 1));
}

void output_one(Instruction* ins) {
  if (ins->opcode == AT) {
    printf("@%s\n", ins->label);
  } else if (ins->opcode == BAR) {
    printf("|%04x\n", ins->literal);
  } else if (ins->opcode == SEMI) {
    printf("  ;%s\n", ins->label);
  } else if (ins->opcode == JCI) {
    printf("  ?%s\n", ins->label);
  } else if (ins->opcode == JMI) {
    printf("  !%s\n", ins->label);
  } else if (ins->opcode == JSI) {
    printf("  %s\n", ins->label);
  } else if (ins->opcode == LIT) {
    printf("  #%02x\n", ins->literal);
  } else if (ins->opcode == LIT2) {
    printf("  #%04x\n", ins->literal);
  } else if (ins->opcode == LIT2r) {
    printf("  LIT2r %04x\n", ins->literal);
  } else {
    printf("  %s%s%s%s\n", opcode_names[ins->opcode & 0x1f],
      ins->opcode & flag_2 ? "2" : "",
      ins->opcode & flag_k ? "k" : "",
      ins->opcode & flag_r ? "r" : "");
  }
}

void output(Instruction* prog) {
  while (prog->opcode) {
    output_one(prog);
    prog = prog->next;
  }
}
