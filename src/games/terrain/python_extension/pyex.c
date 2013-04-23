

/* headers */
#include <Python.h>




//PyMethodDef methods[] = {
//  {NULL, NULL},
///};






PyObject *MyCommand(self, args)
  PyObject *self, *args;
  { PyObject *result = NULL;
    long a, b;

    if (PyArg_ParseTuple(args, "ii", &a, &b)) {
      result = Py_BuildValue("i", a + b);
    } /* otherwise there is an error,
       * the exception already raised by PyArg_ParseTuple, and NULL is
       * returned.
       */
    return result;
  }

PyMethodDef methods[] = {
  {"add", MyCommand, METH_VARARGS},
  {NULL, NULL},
};




#ifdef __WATCOMC__
// need to force this name to not have a _ decoaration prefix
#pragma aux initpyex1 "*";
#endif

//extern "C"
__declspec(dllexport) 

void __cdecl initpyex1()
  {
    (void)Py_InitModule("pyex1", methods);
  }



/*
void (*destructor) (PyObject *self);
int (*printfunc) (PyObject *self, FILE *fp, int flags);
PyObject *(*getattrfunc) (PyObject *self, char *attrname);
int (*setattrfunc) (PyObject *self, char *attrname, PyObject *value);
int (*cmpfunc) (PyObject *v, PyObject *w);
PyObject *(*reprfunc) (PyObject *self);
long (*hashfunc) (PyObject *self);
int (*inquiry) (PyObject *self);
int (*coercion) (PyObject **v, **w);
PyObject *(*intargfunc) (PyObject *self, int i);
PyObject *(*intintargfunc) (PyObject *self, int il, int ih);
int (*intobjargproc) (PyObject *self, int i, PyObject *value);
int (*intintobjargproc) (PyObject *self, int il, int ih, PyObject *value);
int (*objobjargproc) (PyObject *self, PyObject *i, PyObject *value);
PyObject *(*unaryfunc) (PyObject *self);
PyObject *(*binaryfunc) (PyObject *self, PyObject *obj);
PyObject *(*ternaryfunc) (PyObject *self, PyObject *args, PyObject *keywds);
*/

