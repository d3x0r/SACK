#include "genx.h"

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);

  genxStartDocFile(w, stdout);
  genxStartElementLiteral(w, NULL, WIDE("greeting"));
  genxAddText(w, WIDE("Hello world!"));
  genxEndElement(w);
  genxEndDocument(w);
}
