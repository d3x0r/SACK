#include "genx.h"

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);
  genxNamespace ns1, ns2;
  
  genxStartDocFile(w, stdout);
  genxStartElementLiteral(w, WIDE("http://example.org/1"), WIDE("greeting"));
  genxAddAttributeLiteral(w, WIDE("http://example.com/zot"), WIDE("type"), WIDE("well-formed"));
  genxAddText(w, WIDE("\nHello world!"));
  genxEndElement(w);
  genxEndDocument(w);
}
  
