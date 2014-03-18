#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <atasmart.h>

#include <Python.h>

#define UNUSED __attribute__ (( __unused__ ))

typedef struct {
    PyObject_HEAD
    SkDisk *d;
    PyObject *attr_parse_callback;
} Smart;

static int       Smart_init(Smart*, PyObject*, PyObject*);
static PyObject *to_human_readable_string(uint64_t pretty_value, SkSmartAttributeUnit pretty_unit);
static PyObject* Smart_get_power_on(Smart*, PyObject*, PyObject*);
static PyObject* Smart_get_power_cycle(Smart*);
static PyObject* Smart_get_bad_sectors(Smart*, PyObject*, PyObject*);
static PyObject* Smart_get_temperature(Smart*, PyObject*, PyObject*);
static PyObject* Smart_smart_is_available(Smart* );
static PyObject* Smart_smart_status(Smart*);
static void      Smart_dealloc(Smart*);
static PyObject* Smart_error;

static char* SMART_DOC_STRING =
    "Python Binding for libatasmart\n"
;

static int Smart_init(Smart* self, PyObject* args, UNUSED PyObject* kargs)
{
    char *device;
    int ret = 0;

    if (!PyArg_ParseTuple(args, "s", &device)) {
        PyErr_Format(Smart_error, "failed to parse device name: (%d) %s", errno, strerror(errno));
        return -1;
    }


    if ((ret = sk_disk_open(device, &(self->d))) < 0) {
        PyErr_Format(Smart_error, "Failed to open disk: (%d) %s", errno, strerror(errno));
        return -1;
    }

    return 0;
}

static PyObject* Smart_read_data(Smart* self)
{
    int ret;
    if ((ret = sk_disk_smart_read_data(self->d)) < 0) {
        PyErr_Format(Smart_error, "Failed to read SMART data: (%d) %s", errno, strerror(errno));
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Smart_close(Smart* self)
{
	if (self->d)
	{
		sk_disk_free(self->d);
		self->d = NULL;
	}

    Py_RETURN_NONE;
}


static void Smart_dealloc(Smart* self)
{
	if (self->d)
	{
		sk_disk_free(self->d);
		self->d = NULL;
	}
}

//Get the power-on time        
static PyObject* Smart_get_power_on(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    uint64_t ms;
    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    if ((ret = sk_disk_smart_get_power_on(self->d, &ms)) < 0) {
        PyErr_Format(Smart_error, "Failed to get power on time: (%d) %s", errno, strerror(errno));
        return NULL;
    }
    if (human_readable && PyObject_IsTrue(human_readable))
        return to_human_readable_string(ms, SK_SMART_ATTRIBUTE_UNIT_MSECONDS);
    else
        return Py_BuildValue("K", (unsigned long long) ms);
}

//Get number of power cycles        
static PyObject* Smart_get_power_cycle(Smart* self)
{
    int ret;
    uint64_t count;

    if ((ret = sk_disk_smart_get_power_cycle(self->d, &count)) < 0) {
        PyErr_Format(Smart_error, "Failed to get number of power cycles: (%d) %s", errno, strerror(errno));
        return NULL;
    }
    return Py_BuildValue("K", (unsigned long long) count);
}

//Get the number of bad sectors (i.e. pending and reallocated)    
static PyObject* Smart_get_bad_sectors(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    uint64_t sectors;
    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    if ((ret = sk_disk_smart_get_bad(self->d, &sectors)) < 0) {
        if (errno == 2) {            
        Py_INCREF(Py_None);
        return Py_None;
        }
        PyErr_Format(Smart_error, "Failed to get number of bad sectors: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    if (human_readable && PyObject_IsTrue(human_readable))
        return to_human_readable_string(sectors, SK_SMART_ATTRIBUTE_UNIT_SECTORS);
    else
        return Py_BuildValue("K", (unsigned long long) sectors);
}

static PyObject* Smart_get_temperature(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    uint64_t mkelvin;
    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    if ((ret = sk_disk_smart_get_temperature(self->d, &mkelvin)) < 0) {
        PyErr_Format(Smart_error, "Failed to get disk temperature: (%d) %s", errno, strerror(errno));
        return NULL;
    }
    if (human_readable && PyObject_IsTrue(human_readable))
        return to_human_readable_string(mkelvin, SK_SMART_ATTRIBUTE_UNIT_MKELVIN);
    else
        return Py_BuildValue("d", ((double) mkelvin - 273150) / 1000);
}

static PyObject* Smart_smart_is_available(Smart* self)
{
    int ret;
    SkBool available;

    if ((ret = sk_disk_smart_is_available(self->d, &available)) < 0) {
        PyErr_Format(Smart_error, "Unable to check if SMART is available: (%d) %s", errno, strerror(errno));
        return NULL;
    }

    return PyBool_FromLong(available);
}

static PyObject* Smart_smart_status(Smart* self)
{
    int ret;
    SkBool statusGood;

    if ((ret = sk_disk_smart_status(self->d, &statusGood)) < 0) {
        PyErr_Format(Smart_error, "Failed to get SMART status: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    return PyBool_FromLong(statusGood);
}

static PyObject* Smart_check_sleep_mode(Smart* self)
{
    int ret;
    SkBool awake;

    if ((ret = sk_disk_check_sleep_mode(self->d, &awake)) < 0) {
        PyErr_Format(Smart_error, "Failed to check sleep mode: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    return PyBool_FromLong(awake);
}

static PyObject* Smart_identify_is_available(Smart* self)
{
    int ret;
    SkBool available;

    if ((ret = sk_disk_identify_is_available(self->d, &available)) < 0) {
        PyErr_Format(Smart_error, "Failed to check identify data available: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    return PyBool_FromLong(available);
}
static PyObject* Smart_get_overall(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    SkSmartOverall overall;

    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    if ((ret = sk_disk_smart_get_overall(self->d, &overall)) < 0) {
        PyErr_Format(Smart_error, "Failed to get overall status: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    if (human_readable && PyObject_IsTrue(human_readable))
        return Py_BuildValue("s", sk_smart_overall_to_string(overall));
    else
        return PyInt_FromLong(overall);
}

static PyObject* Smart_get_size(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    uint64_t size;

    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    if ((ret = sk_disk_get_size(self->d, &size)) < 0) {
        PyErr_Format(Smart_error, "Failed to get size: (%d) %s", errno, strerror(errno));
        return NULL;  
    }
    if (human_readable && PyObject_IsTrue(human_readable))
        return Py_BuildValue("d", (double) (size/1024/1024));
    else
        return PyInt_FromLong(size);
}

static PyObject *to_human_readable_string(uint64_t pretty_value, SkSmartAttributeUnit pretty_unit) 
{
    PyObject *str = NULL;
    char fmt_value[32];

        switch (pretty_unit) {
                case SK_SMART_ATTRIBUTE_UNIT_MSECONDS:

                        if (pretty_value >= 1000LLU*60LLU*60LLU*24LLU*365LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f years", ((double) pretty_value)/(1000.0*60*60*24*365));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU*60LLU*60LLU*24LLU*30LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f months", ((double) pretty_value)/(1000.0*60*60*24*30));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU*60LLU*60LLU*24LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f days", ((double) pretty_value)/(1000.0*60*60*24));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU*60LLU*60LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f h", ((double) pretty_value)/(1000.0*60*60));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU*60LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f min", ((double) pretty_value)/(1000.0*60));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU)
                        {
                                PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f s", ((double) pretty_value)/(1000.0));
                                str = PyString_FromFormat(fmt_value);
                        }
                        else
                        {
                                str = PyString_FromFormat("%llu ms", (unsigned long long) pretty_value);
                        }

                        break;

                case SK_SMART_ATTRIBUTE_UNIT_MKELVIN:
                        PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.1f C", ((double) pretty_value - 273150) / 1000);
                        str = PyString_FromFormat(fmt_value);
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_SECTORS:
                        str = PyString_FromFormat("%llu sectors", (unsigned long long) pretty_value);
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_PERCENT:
                        str = PyString_FromFormat("%llu%%", (unsigned long long) pretty_value);
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_SMALL_PERCENT:
                        PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.3f%%", (double) pretty_value);
                        str = PyString_FromFormat(fmt_value);
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_MB:
                        if (pretty_value >= 1000000LLU)
                        {
                          PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.3f TB",  (double) pretty_value / 1000000LLU);
                            str = PyString_FromFormat(fmt_value);
                        }
                        else if (pretty_value >= 1000LLU)
                        {
                          PyOS_snprintf(fmt_value, sizeof(fmt_value), "%0.3f GB",  (double) pretty_value / 1000LLU);
                            str = PyString_FromFormat(fmt_value);
                        }
                        else
                        {
                          str = PyString_FromFormat("%llu MB", (unsigned long long) pretty_value);
                        }
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_NONE:
                        str = PyString_FromFormat("%llu", (unsigned long long) pretty_value);
                        break;

                case SK_SMART_ATTRIBUTE_UNIT_UNKNOWN:
                        str = PyString_FromFormat("n/a");
                        break;

                case _SK_SMART_ATTRIBUTE_UNIT_MAX:
                        assert(FALSE);
        }

        return str;
}

static void _disk_dump_attributes(SkDisk *d, const SkSmartAttributeParsedData *a, void* userdata) {
    PyObject* attr_dict = userdata;
    PyObject* dict = NULL;

    if (!PyDict_CheckExact(attr_dict))
    {
            PyErr_SetString(Smart_error, "Dict Expected");
            return;
    }

    dict = PyDict_New();

	if (!a)
	{
		return;
	}

    if(a->name) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "name"), 
            Py_BuildValue("s", a->name));
    }

    if (a->current_value_valid) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "value"), 
            Py_BuildValue("b", a->current_value));

    } else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "value"), 
            Py_BuildValue("O", Py_None));
        
    }

    if (a->worst_value_valid) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "worst"), 
            Py_BuildValue("b", a->worst_value));

    } else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "worst"), 
            Py_BuildValue("O", Py_None));        
    }

    if (a->threshold_valid) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "threshold"), 
            Py_BuildValue("b", a->threshold));

    } else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "threshold"), 
            Py_BuildValue("O", Py_None));        
    }

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "unit"), 
        Py_BuildValue("b", a->pretty_unit));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "human_readable"), 
        Py_BuildValue("O", to_human_readable_string(a->pretty_value, a->pretty_unit)));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "raw"), 
        Py_BuildValue("bbbbbb", a->raw[0], a->raw[1], a->raw[2], a->raw[3], a->raw[4], a->raw[5]));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "updates"), 
        Py_BuildValue("O", a->online ? Py_True : Py_False));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "warn"), 
        Py_BuildValue("O",  a->warn ? Py_True : Py_False));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "flags"), 
        Py_BuildValue("H", a->flags));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "type"), 
        Py_BuildValue("s",  a->prefailure ? "prefail" : "old-age"));

      if (a->current_value && a->threshold) {
          if ( a->current_value <= a->threshold) {
            PyDict_SetItem(dict, 
                Py_BuildValue("s", "failed"), 
                Py_BuildValue("O",  Py_True));

          }  else {
            PyDict_SetItem(dict, 
                Py_BuildValue("s", "failed"), 
                Py_BuildValue("O",  Py_False));

          }
      }  else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "failed"), 
            Py_BuildValue("O",  Py_None));

      }

    if (a->good_now_valid) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "good"), 
            Py_BuildValue("O", PyBool_FromLong(a->good_now_valid)));

    } else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "good"), 
            Py_BuildValue("O", Py_None));        
    }

    if (a->good_in_the_past_valid) {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "past"), 
            Py_BuildValue("O", PyBool_FromLong(a->good_in_the_past_valid)));

    } else {
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "past"), 
            Py_BuildValue("O", Py_None));        
    }

    PyDict_SetItem(attr_dict, 
        Py_BuildValue("b", a->id),
		dict);
    Py_INCREF(dict);
/*
    if (!PyList_Append(attr_list, dict))
    {
            PyErr_SetString(Smart_error, "List Error");
            return NULL;        
    }
    */
}


static PyObject* Smart_get_attributes(Smart* self)
{
    int ret;
    PyObject *attr_dict = NULL;

    attr_dict = PyDict_New();

    if ((ret = sk_disk_smart_parse_attributes(self->d, _disk_dump_attributes, attr_dict)) < 0)
    {
        PyErr_SetString(Smart_error, "SMART Attribute parsing error");
        return NULL;
    }

    return attr_dict;
}

static PyObject* Smart_get_info(Smart* self, PyObject* args, PyObject* kwargs)
{
    int ret;
    const SkSmartParsedData *spd;

    PyObject* dict = NULL;
    PyObject* human_readable = NULL;

    static char *kwlist[] = {"human_readable", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &human_readable))
        return NULL;

    dict = PyDict_New();


    if ((ret = sk_disk_smart_parse(self->d, &spd)) < 0)
    {
        PyErr_SetString(Smart_error, "SMART info parsing error");
        return NULL;
    }

    if (human_readable && PyObject_IsTrue(human_readable))
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "offline_data_collection_status"), 
            Py_BuildValue("s", sk_smart_offline_data_collection_status_to_string(spd->offline_data_collection_status)));
    else
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "offline_data_collection_status"), 
            Py_BuildValue("K", spd->offline_data_collection_status));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "total_offline_data_collection_seconds"), 
        Py_BuildValue("K", spd->total_offline_data_collection_seconds));

    if (human_readable && PyObject_IsTrue(human_readable))
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "self_test_execution_status"), 
            Py_BuildValue("s", sk_smart_self_test_execution_status_to_string(spd->offline_data_collection_status)));
    else
        PyDict_SetItem(dict, 
            Py_BuildValue("s", "self_test_execution_status"), 
            Py_BuildValue("K", spd->self_test_execution_status));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "self_test_execution_percent_remaining"), 
        Py_BuildValue("K", spd->self_test_execution_percent_remaining));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "conveyance_test_available"), 
        Py_BuildValue("O", PyBool_FromLong(spd->conveyance_test_available)));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "short_and_extended_test_available"), 
        Py_BuildValue("O", PyBool_FromLong(spd->short_and_extended_test_available)));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "start_test_available"), 
        Py_BuildValue("O", PyBool_FromLong(spd->start_test_available)));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "abort_test_available"), 
        Py_BuildValue("O", PyBool_FromLong(spd->abort_test_available)));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "short_test_polling_minutes"), 
        Py_BuildValue("K", spd->short_test_polling_minutes));
    PyDict_SetItem(dict, 
        Py_BuildValue("s", "extended_test_polling_minutes"), 
        Py_BuildValue("K", spd->extended_test_polling_minutes));
    PyDict_SetItem(dict, 
        Py_BuildValue("s", "conveyance_test_polling_minutes"), 
        Py_BuildValue("K", spd->conveyance_test_polling_minutes));

    return dict;
}

static PyObject* Smart_get_identify(Smart* self)
{
    int ret;
    const SkIdentifyParsedData *ipd;

    PyObject* dict = NULL;

    dict = PyDict_New();


    if ((ret = sk_disk_identify_parse(self->d, &ipd)) < 0)
    {
	    PyErr_SetString(Smart_error, "SMART identify  parsing error");
	    return NULL;
    }

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "model"), 
        Py_BuildValue("s", ipd->model));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "serial"), 
        Py_BuildValue("s", ipd->serial));

    PyDict_SetItem(dict, 
        Py_BuildValue("s", "firmware"), 
        Py_BuildValue("s", ipd->firmware));

    return dict;
}

static PyObject *Smart_self_test(Smart *self, PyObject* args, PyObject* kwargs)
{
    int ret;
    char test_type = 0;

    static char *kwlist[] = {"test_type", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "b", kwlist, &test_type))
        return NULL;

    if ((ret = sk_disk_smart_self_test(self->d, test_type)) < 0) {
        PyErr_Format(Smart_error, "Failed to read SMART data: (%d) %s", errno, strerror(errno));
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}
/*
static void
_parse_attr_cb (SkDisk                           *d,
               const SkSmartAttributeParsedData *a,
               void                             *user_data)
{
  PyObject* data = user_data;
  gboolean failed = FALSE;
  gboolean failed_in_the_past = FALSE;
  gint current, worst, threshold;

  current =   a->current_value_valid ? a->current_value : -1;
  worst =     a->worst_value_valid   ? a->worst_value : -1;
  threshold = a->threshold_valid     ? a->threshold : -1;

  g_variant_builder_add (&data->builder,
                         "(ysqiiixia{sv})",
                         a->id,
                         a->name,
                         a->flags,
                         current,
                         worst,
                         threshold,
                         a->pretty_value,     a->pretty_unit,
                         NULL); 

  if (current > 0 && threshold > 0 && current <= threshold)
    failed = TRUE;

  if (worst > 0 && threshold > 0 && worst <= threshold)
    failed_in_the_past = TRUE;

  if (failed)
    data->num_attributes_failing += 1;

  if (failed_in_the_past)
    data->num_attributes_failed_in_the_past += 1;
}
*/

static PyMethodDef Smart_methods[] = {
    { "check_sleep_mode", (PyCFunction)Smart_check_sleep_mode, METH_NOARGS, "Check if disk is in sleep mode"},
    { "read_data", (PyCFunction)Smart_read_data, METH_NOARGS, "Read SMART data from disk"},
    { "identify_is_available", (PyCFunction)Smart_identify_is_available, METH_NOARGS, "Check identify is available"},
    { "smart_is_available", (PyCFunction)Smart_smart_is_available, METH_NOARGS, "Check if SMART is available" },
    { "smart_status", (PyCFunction)Smart_smart_status, METH_NOARGS, "Get smart status" },
    { "get_attributes", (PyCFunction)Smart_get_attributes, METH_NOARGS, "Get smart attributes" },
    { "get_info", (PyCFunction)Smart_get_info, METH_VARARGS | METH_KEYWORDS, "Get smart information" },
    { "get_identify", (PyCFunction)Smart_get_identify, METH_NOARGS, "Get smart information" },
    { "get_size", (PyCFunction)Smart_get_size, METH_VARARGS | METH_KEYWORDS, "Get smart information" },

    { "get_power_on", (PyCFunction)Smart_get_power_on, METH_VARARGS | METH_KEYWORDS, "Get the disk power-on time"},
    { "get_power_cycle", (PyCFunction)Smart_get_power_cycle, METH_NOARGS, "Get number of power cycles" },
    { "get_bad_sectors", (PyCFunction)Smart_get_bad_sectors, METH_VARARGS | METH_KEYWORDS, "Get number of bad sectors" },
    { "get_temperature", (PyCFunction)Smart_get_temperature, METH_VARARGS | METH_KEYWORDS, "Get the disk temperature" },
    { "get_overall", (PyCFunction)Smart_get_overall, METH_VARARGS | METH_KEYWORDS, "Get overall status" },
    { "self_test", (PyCFunction)Smart_self_test, METH_VARARGS | METH_KEYWORDS, "Initiate Self-test" },
    { "close", (PyCFunction)Smart_close, METH_NOARGS, "Close device" },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject PyType_Smart = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /* ob_size */
    "_atasmart.Smart",                                  /* tp_name */
    sizeof(Smart),                                  /* tp_basicsize */
    0,                                              /* tp_itemsize */
    (destructor)(Smart_dealloc),                    /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_compare */
    0,                                              /* tp_repr */
    0,                                              /* tp_as_number */
    0,                                              /* tp_as_sequence */
    0,                                              /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    0,                                              /* tp_str */
    PyObject_GenericGetAttr,                        /* tp_getattro */
    PyObject_GenericSetAttr,                        /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    0,                                              /* tp_doc */
    0,                                              /* tp_traverse */
    0,                                              /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Smart_methods,                                  /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)(Smart_init),                         /* tp_init */
    0,                                              /* tp_alloc */
    PyType_GenericNew,                              /* tp_new */
    0,                                              /* tp_free */
    0,                                              /* tp_is_gc */
    0,                                              /* tp_bases */
    0,                                              /* tp_mro */
    0,                                              /* tp_cache */
    0,                                              /* tp_subclasses */
    0,                                              /* tp_weaklist */
    0                                               /* tp_del */
};
 
PyMODINIT_FUNC init_atasmart(void)
{
    PyObject* module;

    PyType_Ready(&PyType_Smart);

    module = Py_InitModule3("_atasmart", NULL, SMART_DOC_STRING);
    Smart_error = PyErr_NewException("_atasmart.error", NULL, NULL);

    PyModule_AddIntConstant(module, "OVERALL_GOOD", SK_SMART_OVERALL_GOOD);
    PyModule_AddIntConstant(module, "OVERALL_BAD_ATTRIBUTE_IN_THE_PAST", SK_SMART_OVERALL_BAD_ATTRIBUTE_IN_THE_PAST);
    PyModule_AddIntConstant(module, "OVERALL_BAD_SECTOR", SK_SMART_OVERALL_BAD_SECTOR);
    PyModule_AddIntConstant(module, "OVERALL_BAD_ATTRIBUTE_NOW", SK_SMART_OVERALL_BAD_ATTRIBUTE_NOW);
    PyModule_AddIntConstant(module, "OVERALL_BAD_SECTOR_MANY", SK_SMART_OVERALL_BAD_SECTOR_MANY);
    PyModule_AddIntConstant(module, "OVERALL_BAD_STATUS", SK_SMART_OVERALL_BAD_STATUS);

    PyModule_AddIntConstant(module, "SELF_TEST_SHORT", SK_SMART_SELF_TEST_SHORT);
    PyModule_AddIntConstant(module, "SELF_TEST_EXTENDED", SK_SMART_SELF_TEST_EXTENDED);
    PyModule_AddIntConstant(module, "SELF_TEST_CONVEYANCE", SK_SMART_SELF_TEST_CONVEYANCE);
    PyModule_AddIntConstant(module, "SELF_TEST_ABORT", SK_SMART_SELF_TEST_ABORT);

    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_UNKNOWN", SK_SMART_ATTRIBUTE_UNIT_UNKNOWN);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_NONE", SK_SMART_ATTRIBUTE_UNIT_NONE);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_MSECONDS", SK_SMART_ATTRIBUTE_UNIT_MSECONDS);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_SECTORS", SK_SMART_ATTRIBUTE_UNIT_SECTORS);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_MKELVIN", SK_SMART_ATTRIBUTE_UNIT_MKELVIN);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_SMALL_PERCENT", SK_SMART_ATTRIBUTE_UNIT_SMALL_PERCENT);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_PERCENT", SK_SMART_ATTRIBUTE_UNIT_PERCENT);
    PyModule_AddIntConstant(module, "ATTRIBUTE_UNIT_MB", SK_SMART_ATTRIBUTE_UNIT_MB);

    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_NEVER", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_NEVER);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_SUCCESS", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_SUCCESS);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_INPROGRESS", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_INPROGRESS);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_SUSPENDED", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_SUSPENDED);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_ABORTED", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_ABORTED);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_FATAL", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_FATAL);
    PyModule_AddIntConstant(module, "OFFLINE_DATA_COLLECTION_STATUS_UNKNOWN", SK_SMART_OFFLINE_DATA_COLLECTION_STATUS_UNKNOWN);

    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_SUCCESS_OR_NEVER", SK_SMART_SELF_TEST_EXECUTION_STATUS_SUCCESS_OR_NEVER);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ABORTED", SK_SMART_SELF_TEST_EXECUTION_STATUS_ABORTED);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_INTERRUPTED", SK_SMART_SELF_TEST_EXECUTION_STATUS_INTERRUPTED);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_FATAL", SK_SMART_SELF_TEST_EXECUTION_STATUS_FATAL);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ERROR_UNKNOWN", SK_SMART_SELF_TEST_EXECUTION_STATUS_ERROR_UNKNOWN);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ERROR_ELECTRICAL", SK_SMART_SELF_TEST_EXECUTION_STATUS_ERROR_ELECTRICAL);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ERROR_SERVO", SK_SMART_SELF_TEST_EXECUTION_STATUS_ERROR_SERVO);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ERROR_READ", SK_SMART_SELF_TEST_EXECUTION_STATUS_ERROR_READ);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_ERROR_HANDLING", SK_SMART_SELF_TEST_EXECUTION_STATUS_ERROR_HANDLING);
    PyModule_AddIntConstant(module, "SELF_TEST_EXECUTION_STATUS_INPROGRESS", SK_SMART_SELF_TEST_EXECUTION_STATUS_INPROGRESS);

    Py_INCREF(&PyType_Smart);
    Py_INCREF(Smart_error);

    PyModule_AddObject(module, "Smart", (PyObject*)(&PyType_Smart));
    PyModule_AddObject(module, "error", Smart_error);
}