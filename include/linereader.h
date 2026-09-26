#ifndef APEX_LINEREADER_H
#define APEX_LINEREADER_H

/* Reads a line with interactive line editing (arrows, history navigation,
   cursor positioning, backspace, and tab autocompletion) when connected
   to a terminal, or falls back to standard line reading when piped/scripted. */
char *lsh_read_interactive_line(void);

#endif /* APEX_LINEREADER_H */
