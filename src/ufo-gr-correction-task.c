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

#include "ufo-gr-correction-task.h"


struct _UfoGrCorrectionTaskPrivate {
    gfloat *corrected_flats;
    gboolean fix_nan_and_inf;
    guint n_flat_fields;
    guint counter;
    gsize n_pixels_image;
    gsize n_total_bytes;
    guint n_outputs;
    guint output_counter;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGrCorrectionTask, ufo_gr_correction_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_GR_CORRECTION_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GR_CORRECTION_TASK, UfoGrCorrectionTaskPrivate))

enum {
    PROP_0,
    PROP_FIX_NAN_AND_INF,
    PROP_NUM_FLAT_FIELDS,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

UfoNode *
ufo_gr_correction_task_new (void)
{
    return UFO_NODE (g_object_new (UFO_TYPE_GR_CORRECTION_TASK, NULL));
}

static void
ufo_gr_correction_task_setup (UfoTask *task,
                              UfoResources *resources,
                              GError **error)
{
    UfoGrCorrectionTaskPrivate *priv;
    priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (task);
}

static void
ufo_gr_correction_task_get_requisition (UfoTask *task,
                                        UfoBuffer **inputs,
                                        UfoRequisition *requisition)
{
    UfoGrCorrectionTaskPrivate *priv;
    UfoRequisition in_req;

    priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (task);
    ufo_buffer_get_requisition (inputs[1], &in_req);

    requisition->n_dims = 3;
    requisition->dims[0] = in_req.dims[0];
    requisition->dims[1] = in_req.dims[1];
    requisition->dims[2] = priv->n_flat_fields;

    priv->n_pixels_image = requisition->dims[0] * requisition->dims[1];
    priv->n_total_bytes = sizeof(gfloat) * requisition->dims[0] * requisition->dims[1] * requisition->dims[2];
    priv->corrected_flats = (gfloat *) g_malloc0(priv->n_total_bytes);
}

static guint
ufo_gr_correction_task_get_num_inputs (UfoTask *task)
{
    return 2;
}

static guint
ufo_gr_correction_task_get_num_dimensions (UfoTask *task, guint input)
{
    return 3;
}

static UfoTaskMode
ufo_gr_correction_task_get_mode (UfoTask *task)
{
    return UFO_TASK_MODE_REDUCTOR | UFO_TASK_MODE_CPU;
}

static gboolean
ufo_gr_correction_task_process (UfoTask *task,
                                UfoBuffer **inputs,
                                UfoBuffer *output,
                                UfoRequisition *requisition)
{
    UfoGrCorrectionTaskPrivate *priv;
    UfoProfiler *profiler;
    gfloat *dark_data;
    gfloat *flat_data;

    gfloat corrected_value;

    priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (task);

    if (priv->counter >= priv->n_flat_fields)
        return FALSE;

    dark_data = ufo_buffer_get_host_array (inputs[0], NULL);
    flat_data = ufo_buffer_get_host_array (inputs[1], NULL);

    profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));

    ufo_profiler_start (profiler, UFO_PROFILER_TIMER_CPU);

    for (gsize i = 0; i < priv->n_pixels_image; i++) {
        corrected_value = flat_data[i] - dark_data[i];

        if (priv->fix_nan_and_inf && (isnan (corrected_value) || isinf (corrected_value))) {
            corrected_value = 0.0;
        }
        priv->corrected_flats[priv->counter * priv->n_pixels_image + i] = corrected_value;
    }

    ufo_profiler_stop (profiler, UFO_PROFILER_TIMER_CPU);

    printf("ufo_gr_correction_task_process [counter = %d]\n", priv->counter);

    priv->counter++;
    return TRUE;
}

static gboolean
ufo_gr_correction_task_generate (UfoTask *task,
                                 UfoBuffer *output,
                                 UfoRequisition *requisition)
{
    UfoGrCorrectionTaskPrivate *priv;
    gfloat *out_data;

    priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (task);

    if (priv->output_counter == priv->n_outputs) {
        return FALSE;
    }

    printf("ufo_gr_correction_task_generate  %p\n", task);

    out_data = ufo_buffer_get_host_array (output, NULL);
    memcpy(out_data, priv->corrected_flats, priv->n_total_bytes);

    priv->output_counter++;
    return TRUE;
}

static void
ufo_gr_correction_task_set_property (GObject *object,
                                             guint property_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    UfoGrCorrectionTaskPrivate *priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FIX_NAN_AND_INF:
            priv->fix_nan_and_inf = g_value_get_boolean (value);
            break;
        case PROP_NUM_FLAT_FIELDS:
            priv->n_flat_fields = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gr_correction_task_get_property (GObject *object,
                                             guint property_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    UfoGrCorrectionTaskPrivate *priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FIX_NAN_AND_INF:
            g_value_set_boolean (value, priv->fix_nan_and_inf);
            break;
        case PROP_NUM_FLAT_FIELDS:
            g_value_set_uint (value, priv->n_flat_fields);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ufo_gr_correction_task_finalize (GObject *object)
{
    G_OBJECT_CLASS (ufo_gr_correction_task_parent_class)->finalize (object);
}

static void
ufo_task_interface_init (UfoTaskIface *iface)
{
    iface->setup = ufo_gr_correction_task_setup;
    iface->get_num_inputs = ufo_gr_correction_task_get_num_inputs;
    iface->get_num_dimensions = ufo_gr_correction_task_get_num_dimensions;
    iface->get_mode = ufo_gr_correction_task_get_mode;
    iface->get_requisition = ufo_gr_correction_task_get_requisition;
    iface->process = ufo_gr_correction_task_process;
    iface->generate = ufo_gr_correction_task_generate;
}

static void
ufo_gr_correction_task_class_init (UfoGrCorrectionTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = ufo_gr_correction_task_set_property;
    gobject_class->get_property = ufo_gr_correction_task_get_property;
    gobject_class->finalize = ufo_gr_correction_task_finalize;

    properties[PROP_FIX_NAN_AND_INF] =
        g_param_spec_boolean("fix-nan-and-inf",
                             "Replace NAN and INF values with 0.0",
                             "Replace NAN and INF values with 0.0",
                             FALSE,
                             G_PARAM_READWRITE);

    properties[PROP_NUM_FLAT_FIELDS] = 
        g_param_spec_uint ("num-flat-fields",
                           "Number of flat-fields",
                           "Number of flat-fields",
                           1, G_MAXUINT, 1,
                           G_PARAM_READWRITE);


    for (guint i = PROP_0 + 1; i < N_PROPERTIES; i++)
        g_object_class_install_property (gobject_class, i, properties[i]);

    g_type_class_add_private (gobject_class, sizeof(UfoGrCorrectionTaskPrivate));
}

static void
ufo_gr_correction_task_init(UfoGrCorrectionTask *self)
{
    self->priv = UFO_GR_CORRECTION_TASK_GET_PRIVATE(self);
    self->priv->fix_nan_and_inf = FALSE;
    self->priv->n_flat_fields = 1;
    self->priv->counter = 0;
    self->priv->n_outputs = 1;
    self->priv->output_counter = 0;
}
