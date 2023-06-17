#include "chibi.h"

Instruction *emit(Instruction *head, char* opcode, int literal, char* label) {
  strncpy(head->opcode, opcode, sizeof(head->opcode));
  head->literal = literal;
  if (label)
    head->label = strdup(label);
  head->next = calloc(1, sizeof(Instruction));
  return head->next;
}

#define ConstantFolding(o, x) \
    if (!strcmp(prog->opcode, "LIT2") \
        && !strcmp(prog->next->opcode, "LIT2") \
        && !strcmp(prog->next->next->opcode, o)) { \
      prog->literal x prog->next->literal; \
      prog->next = prog->next->next->next; \
      changed = true; \
      continue; \
    }

static bool optimize_pass(Instruction* prog, int stage) {
  bool changed = false;
  while (prog->opcode[0]) {
    if (prog->next && prog->next->next) {
      ConstantFolding("ADD2", +=);
      ConstantFolding("SUB2", -=);
      ConstantFolding("MUL2", *=);
      ConstantFolding("DIV2", /=);
      ConstantFolding("AND2", &=);
      ConstantFolding("ORA2", |=);
      ConstantFolding("EOR2", ^=);
    }

    // Fold INC2
    if (!strcmp(prog->opcode, "LIT2") && prog->next && !strcmp(prog->next->opcode, "INC2")) {
      ++prog->literal;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // Shrink literals after constant-folding
    if (stage >= 1 && !strcmp(prog->opcode, "LIT2") && prog->next && !strcmp(prog->next->opcode, "NIP")) {
      strcpy(prog->opcode, "LIT");
      prog->literal &= 0xff;
      prog->next = prog->next->next;
      changed = true;
      continue;
    }

    // Example: DUP2 DUP2 -> DUP2k
    if (!strcmp(prog->opcode, "DUP2") && prog->next && !strcmp(prog->next->opcode, "DUP2")) {
      strcpy(prog->opcode, "DUP2k");
      prog->next = prog->next->next;
      changed = true;
      continue;
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
  if (ins->opcode[0] == '@') {
    printf("@%s\n", ins->label);
  } else if (ins->opcode[0] == '|') {
    printf("|%04x\n", ins->literal);
  } else if (strlen(ins->opcode) == 1) { // ;, ?, !, space (function call)
    printf("  %s%s\n", ins->opcode, ins->label);
  } else if (!strcmp(ins->opcode, "LIT2")) {
    printf("  #%04x\n", ins->literal);
  } else if (!strcmp(ins->opcode, "LIT")) {
    assert(ins->literal < 0x100);
    printf("  #%02x\n", ins->literal);
  } else {
    printf("  %s\n", ins->opcode);
  }
}

void output(Instruction* prog) {
  while (prog->opcode[0]) {
    output_one(prog);
    prog = prog->next;
  }
}

void proof_of_concept(void) {
  Instruction out = {};
  Instruction *head = &out;
  head = emit(head, "LIT2", 1, "");
  head = emit(head, "LIT2", 2, "");
  head = emit(head, "ADD2", 0, "");
  head = emit(head, "LIT2", 3, "");
  head = emit(head, "LIT2", 4, "");
  head = emit(head, "ADD2", 0, "");
  head = emit(head, "MUL2", 0, "");
  head = emit(head, "NIP", 0, "");
  head = emit(head, "DUP2", 0, "");
  head = emit(head, "DUP2", 0, "");
  optimize(&out);
  output(&out);
}
