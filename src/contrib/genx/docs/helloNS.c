#include "genx.h"

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);
  genxNamespace ns1, ns2;
  
  genxStartDocFile(w, stdout);
  genxStartElementLiteral(w, "http://example.org/1", "greeting");
  genxAddAttributeLiteral(w, "http://example.com/zot", "type", "well-formed");
  genxAddText(w, "\nHello world!");
  genxEndElement(w);
  genxEndDocument(w);
}
  
