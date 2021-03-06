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

#include <stdio.h>
#include <math.h>

#include "ufo-swap-quadrants-task.h"

/**
 * SECTION:ufo-swap_quadrants-task
 * @Short_description: Write TIFF files
 * @Title: swap_quadrants
 *
 */

struct _UfoSwapQuadrantsTaskPrivate {
    UfoResources *resources;
    cl_kernel swap_quadrants_kernel_real;
    cl_kernel swap_quadrants_kernel_complex;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoSwapQuadrantsTask, ufo_swap_quadrants_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_SWAP_QUADRANTS_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_SWAP_QUADRANTS_TASK, UfoSwapQuadrantsTaskPrivate))

enum {
    PROP_0,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_swap_quadrants_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_SWAP_QUADRANTS_TASK, NULL));
}

static void
ufo_swap_quadrants_task_setup (UfoTask *task,
                               UfoResources *resources,
                               GError **error)
{
    UfoSwapQuadrantsTaskPrivate *priv = UFO_SWAP_QUADRANTS_TASK_GET_PRIVATE (task);
    priv->resources = resources;
    priv->swap_quadrants_kernel_real = ufo_resources_get_kernel(resources, "swap_quadrants_kernel.cl", "swap_quadrants_kernel_real", error);
    priv->swap_quadrants_kernel_complex = ufo_resources_get_kernel(resources, "swap_quadrants_kernel.cl", "swap_quadrants_kernel_complex", error);
}

static void
ufo_swap_quadrants_task_get_requisition (UfoTask *task,
                                         UfoBuffer **inputs,
                                         UfoRequisition *requisition)
{
    UfoRequisition input_requisition;
    ufo_buffer_get_requisition (inputs[0], &input_requisition);

    requisition->n_dims = input_requisition.n_dims;
    requisition->dims[0] = input_requisition.dims[0];
    requisition->dims[1] = input_requisition.dims[1];
}

static guint
ufo_swap_quadrants_task_get_num_inputs (UfoTask *task)
{
    return 1;
}

static guint
ufo_swap_quadrants_task_get_num_dimensions (UfoTask *task,
                               guint input)
{
    g_return_val_if_fail (input == 0, 0);
    return 2;
}

static UfoTaskMode
ufo_swap_quadrants_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}

static gboolean
ufo_swap_quadrants_task_process (UfoTask *task,
                                 UfoBuffer **inputs,
                                 UfoBuffer *output,
                                 UfoRequisition *requisition)
{
    UfoSwapQuadrantsTaskPrivate *priv;
    UfoGpuNode *node;
    UfoProfiler *profiler;
    cl_command_queue cmd_queue;
    cl_mem in_mem;
    cl_mem out_mem;

    priv = UFO_SWAP_QUADRANTS_TASK_GET_PRIVATE (task);
    node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (task)));
    cmd_queue = ufo_gpu_node_get_cmd_queue (node);

    UfoRequisition input_requisition;
    ufo_buffer_get_requisition (inputs[0], &input_requisition);

    /* args */
    in_mem = ufo_buffer_get_device_array (inputs[0], cmd_queue);
    out_mem = ufo_buffer_get_device_array (output, cmd_queue);

    size_t local_work_size[] = {16,16};
    size_t working_size[2];
    cl_kernel used_kernel;

    if (requisition->dims[0]/2 == requisition->dims[1]) { //if complex
        working_size[0] = requisition->dims[0]/2;
        working_size[1] = requisition->dims[1]/2;
        used_kernel = priv->swap_quadrants_kernel_complex;
    }
    else { //if real
        working_size[0] = requisition->dims[0];
        working_size[1] = requisition->dims[1]/2;
        used_kernel = priv->swap_quadrants_kernel_real;
    }
    /* execution */
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (used_kernel, 0, sizeof (cl_mem), &in_mem));
    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (used_kernel, 1, sizeof (cl_mem), &out_mem));

    profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));
    ufo_profiler_call (profiler, cmd_queue, used_kernel, requisition->n_dims, working_size, local_work_size);
    
    return TRUE;
}

static void
ufo_swap_quadrants_task_set_property (GObject *object,
                                      guint property_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    switch (property_id) {    /*
    UFO_RESOURCES_CHECK_CLERR (clEnqueueNDRangeKernel (cmd_queue,
                                            priv->dfi_sinc_kernel,
                                            requisition->n_dims,
                                            NULL,
                                            working_size,
                                            local_work_size,
                                            0, NULL, NULL));
    */
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_swap_quadrants_task_get_property (GObject *object,
                                      guint property_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_swap_quadrants_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_swap_quadrants_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_swap_quadrants_task_setup;
    iface->get_num_inputs = ufo_swap_quadrants_task_get_num_inputs;
    iface->get_num_dimensions = ufo_swap_quadrants_task_get_num_dimensions;
    iface->get_mode = ufo_swap_quadrants_task_get_mode;
    iface->get_requisition = ufo_swap_quadrants_task_get_requisition;
    iface->process = ufo_swap_quadrants_task_process;
}

static void
ufo_swap_quadrants_task_class_init (UfoSwapQuadrantsTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_swap_quadrants_task_set_property;
    gobject_class->get_property = ufo_swap_quadrants_task_get_property;
    gobject_class->finalize = ufo_swap_quadrants_task_finalize;

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoSwapQuadrantsTaskPrivate));
}

static void
ufo_swap_quadrants_task_init(UfoSwapQuadrantsTask *self)
{
    self->priv = UFO_SWAP_QUADRANTS_TASK_GET_PRIVATE(self);
}
