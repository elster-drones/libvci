// Copyright (c) 2018-2021, AT&T Intellectual Property.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.1-only

#include <Python.h>
#include "../../vci.hpp"

class OwnedPyObject {
	//This class simplifies reference cleanup of python objects.
public:
	OwnedPyObject() : OwnedPyObject(NULL) {}
	OwnedPyObject(PyObject *obj) : _obj(obj){}
	~OwnedPyObject() {
		Py_XDECREF(_obj);
	}
	PyObject *get() { return _obj; }
private:
	PyObject *_obj;
};

class GILEnsure {
public:
	GILEnsure() {
		_gstate = PyGILState_Ensure();
	}
	~GILEnsure() {
		PyGILState_Release(_gstate);
	}
private:
	PyGILState_STATE _gstate;
};

long py_callable_num_args(PyObject *obj) {
	OwnedPyObject code = PyObject_GetAttrString(obj, "__code__");
	OwnedPyObject argcount = PyObject_GetAttrString(code.get(), "co_argcount");
	return PyLong_AsLong(argcount.get());
}

int py_object_is_rpc_method(PyObject *obj) {
	return PyCallable_Check(obj) && py_callable_num_args(obj) == 1;
}

int py_object_is_rpc_meta_method(PyObject *obj) {
	return PyCallable_Check(obj) && py_callable_num_args(obj) == 2;
}

std::string py_str_to_string(PyObject *obj) {
	std::string out;
	if (obj == NULL) {
		return "";
	}
	OwnedPyObject str_obj = PyUnicode_AsUTF8String(obj);
	char *str = PyBytes_AsString(str_obj.get());
	if (str != NULL) {
		out = str;
	}
	return out;
}

PyObject *py_decode_object(const std::string &encoded_input) {
	PyObject* main = PyImport_AddModule("__main__");//Borrowed ref
	PyObject* globals = PyModule_GetDict(main); //Borrowed ref
	OwnedPyObject locals = PyDict_New();
	OwnedPyObject value = PyUnicode_FromString(encoded_input.c_str());
	PyDict_SetItemString(locals.get(), "encoded_data", value.get());

	OwnedPyObject impret = PyRun_String(
		"import json", Py_single_input, globals, locals.get());

	PyObject* output = PyRun_String(
		"json.loads(encoded_data)", Py_eval_input, globals, locals.get());

	PyDict_Clear(locals.get());
	return output;
}

std::string py_encode_object(PyObject *obj) {
	PyObject* main = PyImport_AddModule("__main__");//Borrowed ref
	PyObject* globals = PyModule_GetDict(main); //Borrowed ref
	OwnedPyObject locals = PyDict_New();
	PyDict_SetItemString(locals.get(), "obj_to_encode", obj);

	OwnedPyObject impret = PyRun_String(
		"import json", Py_single_input, globals, locals.get());

	OwnedPyObject output = PyRun_String(
		"json.dumps(obj_to_encode)", Py_eval_input, globals, locals.get());

	auto out = py_str_to_string(output.get());
	PyDict_Clear(locals.get());
	return out;
}

PyObject *py_get_vci_ex_type() {
	PyObject* main = PyImport_AddModule("__main__");//Borrowed ref
	PyObject* globals = PyModule_GetDict(main); //Borrowed ref
	OwnedPyObject locals = PyDict_New();

	OwnedPyObject ret = PyRun_String(
		"import vci", Py_single_input, globals, locals.get());

	PyObject *output = PyRun_String(
		"vci.Exception", Py_eval_input, globals, locals.get());

	PyDict_Clear(locals.get());
	return output;
}

PyObject *vci_ex_to_py_vci_ex(const vci::Exception& ex) {
	OwnedPyObject py_exception_type = py_get_vci_ex_type();
	return PyObject_CallFunction(py_exception_type.get(), "sss",
		ex.app_tag().c_str(),
		ex.info().c_str(),
		ex.path().c_str());
}

vci::Exception py_vci_ex_to_vci_ex(PyObject *obj) {
	return vci::Exception(
		py_str_to_string(PyObject_CallMethod(obj, "app_tag", "")),
		py_str_to_string(PyObject_CallMethod(obj, "info", "")),
		py_str_to_string(PyObject_CallMethod(obj, "path", "")));
}

void py_handle_ex() {
	auto cppex = vci::Exception("unknown-error", "", "");

	PyObject *_extype, *_exvalue, *_extraceback;
	PyErr_Fetch(&_extype, &_exvalue, &_extraceback);
	PyErr_NormalizeException(&_extype, &_exvalue, &_extraceback);
	OwnedPyObject extype = _extype;
	OwnedPyObject exvalue = _exvalue;
	OwnedPyObject extraceback = _extraceback;
	OwnedPyObject vciType = py_get_vci_ex_type();

	if (PyObject_IsInstance(exvalue.get(), vciType.get()) == 1) {
		cppex = py_vci_ex_to_vci_ex(exvalue.get());
	} else {
		OwnedPyObject ex_str = PyObject_Str(exvalue.get());
		std::string msg = py_str_to_string(ex_str.get());
		cppex = vci::Exception("unknown-error", msg, "");
	}

	throw cppex;
}

class PyMethod : public vci::Method {
public:
	PyMethod(PyObject *func) : _func(func) {}
	std::string operator()(const std::string &encoded_input) {
		auto gil = GILEnsure();
		OwnedPyObject inobj = py_decode_object(encoded_input);
		OwnedPyObject out = PyObject_CallFunctionObjArgs(
			this->_func.get(), inobj.get(), NULL);
		if (PyErr_Occurred() != NULL) {
			py_handle_ex();
		}
		std::string str = py_encode_object(out.get());
		return str;
	}
private:
    OwnedPyObject _func;
};

vci::Method *py_gen_method(PyObject *func) {
	return new PyMethod(func);
}

class PyMethodMeta : public vci::MethodMeta {
public:
	PyMethodMeta(PyObject *func) : _func(func) {}
	std::string operator()(const std::string &encoded_meta,
						   const std::string &encoded_input) {
		auto gil = GILEnsure();
		OwnedPyObject metaobj = py_decode_object(encoded_meta);
		OwnedPyObject inobj = py_decode_object(encoded_input);
		OwnedPyObject out = PyObject_CallFunctionObjArgs(
			this->_func.get(), metaobj.get(), inobj.get(), NULL);
		if (PyErr_Occurred() != NULL) {
			py_handle_ex();
		}
		std::string str = py_encode_object(out.get());
		return str;
	}
private:
    OwnedPyObject _func;
};

vci::MethodMeta *py_gen_method_meta(PyObject *func) {
	return new PyMethodMeta(func);
}

class PySubscriber : public vci::Subscriber {
public:
	PySubscriber(PyObject *func) : _func(func) {}
	void operator()(const std::string &encoded_input) {
		auto gil = GILEnsure();
		OwnedPyObject inobj = py_decode_object(encoded_input);
		OwnedPyObject out = PyObject_CallFunctionObjArgs(
			this->_func.get(), inobj.get(), NULL);
		if (PyErr_Occurred() != NULL) {
			try {
				py_handle_ex();
			} catch (const vci::Exception &e) {
				SWIG_Python_Raise(
					SWIG_NewPointerObj(
						(new vci::Exception(
							static_cast<const vci::Exception& >(e))),
						SWIGTYPE_p_vci__Exception, SWIG_POINTER_OWN),
					"vci::Exception", SWIGTYPE_p_vci__Exception);
			}
		}
	}
private:
    OwnedPyObject _func;
};

vci::Subscriber *py_gen_subscriber(PyObject *func) {
	return new PySubscriber(func);
}
