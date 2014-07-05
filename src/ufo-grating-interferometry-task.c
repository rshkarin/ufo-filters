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
#include <math.h>

#include "clFFT.h"
#include "ufo-grating-interferometry-task.h"

#define MAX_NUM_INPUTS 16

struct _UfoGratingInterferometryTaskPrivate {
    guint n_images;

    cl_kernel kernel_combine;
    cl_kernel kernel_gi;

    clFFT_Plan fft_plan;
    clFFT_Dim3 fft_size;

    cl_context context;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGratingInterferometryTask, ufo_grating_interferometry_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GRATING_INTERFEROMETRY_TASK, UfoGratingInterferometryTaskPrivate))

enum {
    PROP_N_IMAGES,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_grating_interferometry_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_GRATING_INTERFEROMETRY_TASK, NULL));
}

static gboolean
ufo_grating_interferometry_task_process (UfoTask *task,
                         UfoBuffer **inputs,
                         UfoBuffer *output,
                         UfoRequisition *requisition)
{
    UfoGratingInterferometryTaskPrivate *priv;
    UfoGpuNode *node;
    UfoProfiler *profiler;
    cl_command_queue cmd_queue;
    cl_mem in_mem[MAX_NUM_INPUTS];
    cl_mem out_mem;

    priv = UFO_GRATING_INTERFEROMETRY_TASK (task)->priv;
    node = UFO_GPU_NODE (ufo_task_node_get_proc_node (UFO_TASK_NODE (task)));
    cmd_queue = ufo_gpu_node_get_cmd_queue (node);

    for (int i = 0; i < priv->n_images; i++) {
        in_mem[i] = ufo_buffer_get_device_array (inputs[i], cmd_queue);
    }

    out_mem = ufo_buffer_get_device_array (output, cmd_queue);

    for (int i = 0; i < priv->n_images; i++) {
        UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->kernel_combine, i, sizeof (cl_mem), &in_mem[i]));
    }

    UFO_RESOURCES_CHECK_CLERR (clSetKernelArg (priv->kernel_combine, n_images + 1, sizeof (cl_mem), &out_mem));

    profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));
    ufo_profiler_call (profiler, cmd_queue, priv->kernel_combine, 2, requisition->dims, NULL);

    clFFT_ExecuteInterleaved_Ufo (cmd_queue, priv->fft_plan, requisition->dims[0] * requisition->dims[1], clFFT_Forward, out_mem, out_mem, 0, NULL, NULL, profiler);

    return TRUE;
}

static void
ufo_grating_interferometry_task_setup (UfoTask *task,
                       UfoResources *resources,
                       GError **error)
{
    UfoGratingInterferometryTaskPrivate *priv;

    priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (task);

    if (priv->n_images == 4) {
        priv->kernel_combine = ufo_resources_get_kernel (resources, "grating_interferometry.cl", "combine_images4", error);
    }
    else if (priv->n_images == 8) {
        priv->kernel_combine = ufo_resources_get_kernel (resources, "grating_interferometry.cl", "combine_images8", error);
    }
    else if (priv->n_images == 16) {
        priv->kernel_combine = ufo_resources_get_kernel (resources, "grating_interferometry.cl", "combine_images16", error);
    }

    if (priv->kernel_combine != NULL)
        UFO_RESOURCES_CHECK_CLERR (clRetainKernel (priv->kernel_combine));
}

static void
ufo_grating_interferometry_task_get_requisition (UfoTask *task,
                                 UfoBuffer **inputs,
                                 UfoRequisition *requisition)
{
    UfoGratingInterferometryTaskPrivate *priv;
    UfoRequisition input_requisition;

    priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (task);
    ufo_buffer_get_requisition (inputs[0], &input_requisition);

    requisition->n_dims = 1;
    requisition->dims[0] = input_requisition.dims[0] * input_requisition.dims[0] * priv->n_images * 2;
    requisition->dims[1] = 0;

    if (priv->fft_plan == NULL) {
        priv->fft_size.x = priv->n_images;
        priv->fft_size.y = 1
        priv->fft_size.z = 1;

        cl_int cl_err;
        priv->fft_plan = clFFT_CreatePlan (priv->context,
                                           priv->fft_size,
                                           clFFT_1D,
                                           clFFT_InterleavedComplexFormat, 
                                           &cl_err);
        UFO_RESOURCES_CHECK_CLERR (cl_err);
    }
}

static guint
ufo_grating_interferometry_task_get_num_inputs (UfoTask *task)
{
    return MAX_NUM_INPUTS;
}

static guint
ufo_grating_interferometry_task_get_num_dimensions (UfoTask *task,
                                                    guint input)
{
    g_return_val_if_fail (input == 0, 0);
    return 2;
}

static UfoTaskMode
ufo_grating_interferometry_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_PROCESSOR | UFO_TASK_MODE_GPU;
}

static void
ufo_grating_interferometry_task_get_property (GObject *object,
                                              guint property_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    UfoGratingInterferometryTaskPrivate *priv;
    priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_N_IMAGES:
            g_value_set_uint (value, priv->n_images);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_grating_interferometry_task_set_property (GObject *object,
                                              guint property_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    UfoGratingInterferometryTaskPrivate *priv;
    priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_N_IMAGES:
            priv->n_images = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_grating_interferometry_task_finalize (GObject *object)
{
    UfoGratingInterferometryTaskPrivate *priv;

    priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (object);

    clFFT_DestroyPlan (priv->fft_plan);

    if (priv->kernel_combine) {
        clReleaseKernel (priv->kernel_combine);
        priv->kernel_combine = NULL;
    }

    if (priv->kernel_gi) {
        clReleaseKernel (priv->kernel_gi);
        priv->kernel_gi = NULL;
    }

    if (priv->context) {
        UFO_RESOURCES_CHECK_CLERR (clReleaseContext (priv->context));
        priv->context = NULL;
    }

    G_OBJECT_CLASS (ufo_grating_interferometry_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_grating_interferometry_task_setup;
    iface->get_requisition = ufo_grating_interferometry_task_get_requisition;
    iface->get_num_inputs = ufo_grating_interferometry_task_get_num_inputs;
    iface->get_num_dimensions = ufo_grating_interferometry_task_get_num_dimensions;
    iface->get_mode = ufo_grating_interferometry_task_get_mode;
    iface->process = ufo_grating_interferometry_task_process;
}

static void
ufo_grating_interferometry_task_class_init (UfoGratingInterferometryTaskClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);

    oclass->set_property = ufo_grating_interferometry_task_set_property;
    oclass->get_property = ufo_grating_interferometry_task_get_property;
    oclass->finalize = ufo_grating_interferometry_task_finalize;
    
    properties[PROP_N_IMAGES] =
        g_param_spec_uint ("num-images",
            "Number of images",
            "Number of grating images.",
            4, MAX_NUM_INPUTS, 4,
            G_PARAM_READWRITE);

    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (oclass, i, properties[i]);

    g_type_class_add_private(oclass, sizeof(UfoGratingInterferometryTaskPrivate));
}

static void
ufo_grating_interferometry_task_init (UfoGratingInterferometryTask *self)
{
    UfoGratingInterferometryTaskPrivate *priv;
    self->priv = priv = UFO_GRATING_INTERFEROMETRY_TASK_GET_PRIVATE (self);
    priv->kernel_combine = NULL;
    priv->kernel_gi = NULL;
    priv->n_images = 4;
    priv->fft_plan = NULL;
    priv->fft_size.x = 1;
    priv->fft_size.y = 1;
    priv->fft_size.z = 1;
}
