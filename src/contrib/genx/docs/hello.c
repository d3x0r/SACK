#include "genx.h"

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);

  genxStartDocFile(w, stdout);
  genxStartElementLiteral(w, NULL, "greeting");
  genxAddText(w, "Hello world!");
  genxEndElement(w);
  genxEndDocument(w);
}
