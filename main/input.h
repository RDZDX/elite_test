#ifndef input_include_file
#define input_include_file

extern unsigned char yequ, graph, up, down, left, right,
                     enter, math, prgm, vars, clear, mode,
                     del, graphVar, stat, apps, alpha, second;

void updateKeys(void);

char getChar(void);

/* Called from the MRE keyboard callback in main.c */
void input_HandleKeyEvent(VMINT event, VMINT keycode);

#endif
