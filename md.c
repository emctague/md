/* Primitive terminal display program for a limited subset of markdown. */

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void printfmt();
void println(char *line);
void catalloc(char **line, char c);

/* Formatting state */
struct state {
  bool bold;
  bool italic;
  bool title;
};

/* A formatted block. */
struct block {
  struct state state;

  char *text;
  char *url;
  struct block *next;
};

struct block *new_block(struct state state);

int main(int argc, char **argv)
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [file]\n", argv[0]);
    fprintf(stderr, "(`-` or no file means stdin)\n");
    return 1;
  }

  FILE *f;
  if (argc == 1 || !strcmp(argv[1], "-")) f = stdin;
  else f = fopen(argv[1], "r");

  if (!f) {
    fprintf(stderr, "Error: cannot open file %s\n", argv[1]);
    return 2;
  }

  struct state state;
  struct block *root = new_block(state);
  struct block *current = root;

  char *line = NULL;
  size_t n = 0;
  while (getline(&line, &n, f) != -1) {
    state.title = false;

#define PUSH_STATE { struct block *next = new_block(state); current->next = next; current = next; }
    char *ln = line;
    PUSH_STATE;

    // Detect title
    if (*ln == '#') {
      current->state.title = true;
      state.title = true;
      ln++;
    }

    // Turn additional '#' into indentation.
    while (*ln == '#') { catalloc(&current->text, ' '); ln++; }

    // Trim leading whitespace
    while (isspace(*ln) && *ln != '\n') ln++;

    bool prev_star = false;
    while (*ln) {
      bool cur_star = *ln == '*';
      bool skip_add = false;

      if (prev_star) {
        if (cur_star) { state.bold = !state.bold; skip_add = true; PUSH_STATE; }
        else { state.italic = !state.italic; PUSH_STATE; }
        cur_star = false;
      } else {
        if (cur_star) { skip_add = true; }
      }

      prev_star = cur_star;

      if (!skip_add) catalloc(&current->text, *ln);

      ln++;
    }

  }

  current = root->next;
  while (current) {
    if (current->text) {
      printf("\x1b[0m"); // Reset
      if (current->state.title) printf("\x1b[7m"); // Title (Reverse Video)  
      if (current->state.bold) printf("\x1b[1m"); // Bold
      if (current->state.italic) printf("\x1b[3m"); // Italic
      printf("%s", current->text);
    }
    current = current->next;
  }
}

struct block *new_block(struct state state) {
  struct block *block = malloc(sizeof(struct block));
  block->state = state;
  block->text = NULL;
  block->url = NULL;
  block->next = NULL;
  return block;
}

void catalloc(char **text, char c) {
  size_t len = 0;
  if (*text != NULL) len = strlen(*text);
  *text = realloc(*text, len + 2);
  (*text)[len] = c;
  (*text)[len + 1] = '\0';
}
