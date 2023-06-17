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
    if (prog->opcode == LIT2 && prog->next->opcode == LIT2 && prog->next->next->opcode == o) { \
      prog->literal x prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    }

static bool optimize_pass(Instruction* prog) {
  bool changed = false;
  while (prog->opcode) {
    if (prog->next && prog->next->next) {
      ConstantFolding(ADD2, +=);
      ConstantFolding(SUB2, -=);
      ConstantFolding(MUL2, *=);
      ConstantFolding(DIV2, /=);
      ConstantFolding(AND2, &=);
      ConstantFolding(ORA2, |=);
      ConstantFolding(EOR2, ^=);
    }

    // Fold INC2
    if (prog->opcode == LIT2 && prog->next && prog->next->opcode == INC2) {
      ++prog->literal;
      prog->next = prog->next->next;
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

    // Example: DUP2 DUP2 -> DUP2k
    if (prog->opcode == DUP2 && prog->next && prog->next->opcode == DUP2) {
      prog->opcode = DUP2 | flag_k;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    prog = prog->next;
  }
  return changed;
}

void optimize(Instruction* prog) {
  while (optimize_pass(prog));
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
