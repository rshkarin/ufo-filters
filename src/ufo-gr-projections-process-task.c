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

#include "ufo-gr-projections-process-task.h"


struct _UfoGrProjectionsProcessTaskPrivate {
    gfloat *averaged_dark_fields_data;
    gfloat *corrected_flat_data;
    gfloat *projections_data;
    gboolean fix_nan_and_inf;
    guint n_periods;
    guint n_flat_fields;
    guint n_projections;
    guint grating_step;
    guint counter;
    gsize n_pixels_image;
    gsize n_bytes_frame;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGrProjectionsProcessTask, ufo_gr_projections_process_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GR_PROJECTIONS_PROCESS_TASK, UfoGrProjectionsProcessTaskPrivate))

enum {
    PROP_0,
    PROP_FIX_NAN_AND_INF,
    PROP_NUM_PROJECTIONS,
    PROP_GRATING_STEP,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_gr_projections_process_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_GR_PROJECTIONS_PROCESS_TASK, NULL));
}

static void
ufo_gr_projections_process_task_setup (UfoTask *task,
                                      UfoResources *resources,
                                      GError **error)
{
}

static void
ufo_gr_projections_process_task_get_requisition (UfoTask *task,
                                                 UfoBuffer **inputs,
                                                 UfoRequisition *requisition)
{
    UfoGrProjectionsProcessTaskPrivate *priv;
    UfoRequisition in_req;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (task);

    ufo_buffer_get_requisition (inputs[0], &in_req);

    requisition->n_dims = 3;
    requisition->dims[0] = in_req.dims[0];
    requisition->dims[1] = in_req.dims[1];
    requisition->dims[2] = 3; // dpc, dfi, aps

    priv->n_pixels_image = requisition->dims[0] * requisition->dims[1];
    priv->n_flat_fields = in_req.dims[2];
    priv->n_bytes_frame = sizeof(gfloat) * priv->n_pixels_image;
    priv->projections_data = g_malloc0(n_bytes_chunk);
}

static guint
ufo_gr_projections_process_task_get_num_inputs (UfoTask *task)
{
    return 3;
}

static guint
ufo_gr_projections_process_task_get_num_dimensions (UfoTask *task, guint input)
{
    return 3;
}

static UfoTaskMode
ufo_gr_projections_process_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_REDUCTOR_GENERATOR | UFO_TASK_MODE_CPU;
}

static gboolean
ufo_gr_projections_process_task_process (UfoTask *task,
                                UfoBuffer **inputs,
                                UfoBuffer *output,
                                UfoRequisition *requisition)
{
    UfoGrProjectionsProcessTaskPrivate *priv;
    UfoProfiler *profiler;
    gfloat *proj_data;
    gfloat corrected_value;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (task);
    profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));

    printf("ufo_gr_projections_process_task_process [counter = %d , gr-step = %d ]\n", priv->counter, priv->grating_step);

    ufo_profiler_start (profiler, UFO_PROFILER_TIMER_CPU);

    if (priv->counter == priv->grating_step - 1) {
        priv->counter = 0;
        return FALSE;
    }

    if (priv->averaged_dark_fields_data == NULL) {
        priv->averaged_dark_fields_data = ufo_buffer_get_host_array (inputs[1], NULL);
    }

    if (priv->corrected_flat_data == NULL) {
        priv->corrected_flat_data = ufo_buffer_get_host_array (inputs[0], NULL);

        gfloat *flat_fields_mean = (gfloat *)g_malloc0 (sizeof(gfloat) * priv->n_flat_fields);

        for (guint i = 0; i < priv->n_flat_fields; i++) {
            for (gsize j = 0; j < priv->n_pixels_image; j++) {
                flat_fields_mean[i] += (priv->corrected_flat_data[i * priv->n_pixels_image + j]/(gfloat)priv->n_pixels_image);
            }
        }

        //TODO: Perfrom fft of flat fileds mean values flat_fields_mean to get index
    }

    proj_data = ufo_buffer_get_host_array (inputs[2], NULL);

    for (gsize i = 0; i < priv->n_pixels_image; i++) {
        corrected_value = proj_data[i] - priv->averaged_dark_fields_data[i];

        if (priv->fix_nan_and_inf && (isnan (corrected_value) || isinf (corrected_value))) {
            corrected_value = 0.0;
        }

        projections_data[priv->counter * priv->n_pixels_image + i] = corrected_value;
    }

    ufo_profiler_stop (profiler, UFO_PROFILER_TIMER_CPU);

    priv->counter++;
    return TRUE;
}

static gboolean
ufo_gr_projections_process_task_generate (UfoTask *task,
                                 UfoBuffer *output,
                                 UfoRequisition *requisition)
{
    printf("ufo_gr_projections_process_task_generate [GENERATE STUFF]\n");

    //TODO: Get FFT of corrected flat-fields over Z-axis
    //TODO: Calculate contrast images from projections
    //TODO: Correct contrast images 
    //TODO: Perfrom filtering
    //TODO: Filter NaNs nad Infs
    //TODO: Copy 3D image to output buffer

    return TRUE;
}

static void
ufo_gr_projections_process_task_set_property (GObject *object,
                                             guint property_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    UfoGrProjectionsProcessTaskPrivate *priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FIX_NAN_AND_INF:
            priv->fix_nan_and_inf = g_value_get_boolean (value);
            break;
        case PROP_NUM_PROJECTIONS:
            priv->n_projections = g_value_get_uint (value);
            break;
        case PROP_GRATING_STEP:
            priv->grating_step = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gr_projections_process_task_get_property (GObject *object,
                                             guint property_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    UfoGrProjectionsProcessTaskPrivate *priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FIX_NAN_AND_INF:
            g_value_set_boolean (value, priv->fix_nan_and_inf);
            break;
        case PROP_NUM_PROJECTIONS:
            g_value_set_uint (value, priv->n_projections);
            break;
        case PROP_GRATING_STEP:
            g_value_set_uint (value, priv->grating_step);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gr_projections_process_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_gr_projections_process_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_gr_projections_process_task_setup;
    iface->get_num_inputs = ufo_gr_projections_process_task_get_num_inputs;
    iface->get_num_dimensions = ufo_gr_projections_process_task_get_num_dimensions;
    iface->get_mode = ufo_gr_projections_process_task_get_mode;
    iface->get_requisition = ufo_gr_projections_process_task_get_requisition;
    iface->process = ufo_gr_projections_process_task_process;
    iface->generate = ufo_gr_projections_process_task_generate;
}

static void
ufo_gr_projections_process_task_class_init (UfoGrProjectionsProcessTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_gr_projections_process_task_set_property;
    gobject_class->get_property = ufo_gr_projections_process_task_get_property;
    gobject_class->finalize = ufo_gr_projections_process_task_finalize;

    properties[PROP_FIX_NAN_AND_INF] =
        g_param_spec_boolean("fix-nan-and-inf",
                             "Replace NAN and INF values with 0.0",
                             "Replace NAN and INF values with 0.0",
                             FALSE,
                             G_PARAM_READWRITE);

    properties[PROP_NUM_PROJECTIONS] = 
        g_param_spec_uint ("num-projections",
                           "Number of projections",
                           "Number of projections",
                           1, G_MAXUINT, 1,
                           G_PARAM_READWRITE);

    properties[PROP_GRATING_STEP] = 
        g_param_spec_uint ("grating-step",
                           "Step of grating",
                           "Step of grating",
                           1, G_MAXUINT, 1,
                           G_PARAM_READWRITE);


    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoGrProjectionsProcessTaskPrivate));
}

static void
ufo_gr_projections_process_task_init(UfoGrProjectionsProcessTask *self)
{
    self->priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE(self);
    self->priv->n_periods = -1;
    self->priv->corrected_flat_data = NULL;
    self->priv->projections_data = NULL;
    self->priv->averaged_dark_fields_data = NULL;
    self->priv->fix_nan_and_inf = FALSE;
    self->priv->grating_step = 4;
    self->priv->n_flat_fields = 1;
    self->priv->n_projections = self->priv->grating_step;
    self->priv->counter = 0;
}
