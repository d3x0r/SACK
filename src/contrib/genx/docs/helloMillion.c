#include "genx.h"

void oops(genxWriter w)
{
  fprintf(stderr, WIDE("oops %s\n"), genxLastErrorMessage(w));
  exit(1);
}

int main(int argc, char * argv[])
{
  genxWriter w = genxNew(NULL, NULL, NULL);
  genxElement dates, date;
  genxAttribute yyyy, mm;
  genxNamespace ns;
  genxStatus status;
  int i;
  char year[100], month[100];

  if (!(ns = genxDeclareNamespace(w, WIDE("http://example.org/dd"), WIDE("dd"), &status)))
    oops(w);
  if (!(dates = genxDeclareElement(w, ns, WIDE("dates"), &status)))
    oops(w);
  if (!(date = genxDeclareElement(w, NULL, WIDE("date"), &status)))
    oops(w);
  if (!(yyyy = genxDeclareAttribute(w, NULL, WIDE("yyyy"), &status)))
    oops(w);
  if (!(mm = genxDeclareAttribute(w, NULL, WIDE("mm"), &status)))
    oops(w);

  if (genxStartDocFile(w, stdout) ||
      genxStartElement(dates) ||
      genxAddText(w, WIDE("\n")))
    oops(w);
  for (i = 0; i < 1000000; i++)
  {
    sprintf(year, WIDE("%d"), 1900 + (random() % 100));
    sprintf(month, WIDE("%02d"), 1 + (random() % 12));
    if (genxStartElement(date) ||
	genxAddAttribute(yyyy, year) ||
	genxAddAttribute(mm, month) ||
	genxEndElement(w) ||
	genxAddText(w, WIDE("\n ")))
      oops(w);
  }
  if (genxEndElement(w))
    oops(w);
  if (genxEndDocument(w))
    oops(w);
}
  
