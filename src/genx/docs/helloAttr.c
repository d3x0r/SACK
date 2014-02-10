#include "genx.h"

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);

  genxStartDocFile(w, stdout);
  genxStartElementLiteral(w, NULL, WIDE("greeting"));
  genxAddAttributeLiteral(w, NULL, WIDE("type"), WIDE("well-formed")); /* new */
  genxAddText(w, WIDE("Hello world!"));
  genxEndElement(w);
  genxEndDocument(w);
}
  
