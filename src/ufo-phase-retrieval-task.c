/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "ufo-phase-retrieval-task.h"

/**
 * SECTION:ufo-phase-retrieval-task
 * @Short_description: Write TIFF files
 * @Title: phase_retrieval
 *
 */

struct _UfoPhaseRetrievalTaskPrivate {
    gboolean foo;
};

static void ufo_task_interface_init (UfoTaskIface *iface);
static void ufo_gpu_task_interface_init (UfoGpuTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoPhaseRetrievalTask, ufo_phase_retrieval_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init)
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_GPU_TASK,
                                                ufo_gpu_task_interface_init))

#define UFO_PHASE_RETRIEVAL_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_PHASE_RETRIEVAL_TASK, UfoPhaseRetrievalTaskPrivate))

enum {
    PROP_0,
    PROP_TEST,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_phase_retrieval_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_PHASE_RETRIEVAL_TASK, NULL));
}

static void
ufo_phase_retrieval_task_setup (UfoTask *task,
                       UfoResources *resources,
                       GError **error)
{
}

static void
ufo_phase_retrieval_task_get_requisition (UfoTask *task,
                                 UfoBuffer **inputs,
                                 UfoRequisition *requisition)
{
    requisition->n_dims = 0;
}

static void
ufo_phase_retrieval_task_get_structure (UfoTask *task,
                               guint *n_inputs,
                               UfoInputParam **in_params,
                               UfoTaskMode *mode)
{
    *mode = UFO_TASK_MODE_SINGLE;
    *n_inputs = 1;
    *in_params = g_new0 (UfoInputParam, 1);
    (*in_params)[0].n_dims = 2;
}

static gboolean
ufo_phase_retrieval_task_process (UfoGpuTask *task,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition,
                         UfoGpuNode *node)
{
    return TRUE;
}

static void
ufo_phase_retrieval_task_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    UfoPhaseRetrievalTaskPrivate *priv = UFO_PHASE_RETRIEVAL_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_TEST:
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_phase_retrieval_task_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    UfoPhaseRetrievalTaskPrivate *priv = UFO_PHASE_RETRIEVAL_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_TEST:
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_phase_retrieval_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_phase_retrieval_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_phase_retrieval_task_setup;
    iface->get_structure = ufo_phase_retrieval_task_get_structure;
    iface->get_requisition = ufo_phase_retrieval_task_get_requisition;
}

static void
ufo_gpu_task_interface_init (UfoGpuTaskIface *iface)
{
    iface->process = ufo_phase_retrieval_task_process;
}

static void
ufo_phase_retrieval_task_class_init (UfoPhaseRetrievalTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_phase_retrieval_task_set_property;
    gobject_class->get_property = ufo_phase_retrieval_task_get_property;
    gobject_class->finalize = ufo_phase_retrieval_task_finalize;

    properties[PROP_TEST] =
        g_param_spec_string ("test",
            "Test property nick",
            "Test property description blurb",
            "",
            G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoPhaseRetrievalTaskPrivate));
}

static void
ufo_phase_retrieval_task_init(UfoPhaseRetrievalTask *self)
{
    self->priv = UFO_PHASE_RETRIEVAL_TASK_GET_PRIVATE(self);
}