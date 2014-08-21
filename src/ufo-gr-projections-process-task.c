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

#include <fftw3.h>
#include <float.h>
#include <math.h>

struct _UfoGrProjectionsProcessTaskPrivate {
    gfloat *averaged_dark_fields_data;
    gfloat *projections_data;
    gboolean fix_nan_and_inf;
    guint n_periods;
    guint n_flat_fields;
    guint grating_step;
    guint counter;
    guint n_outs;
    gsize n_pixels_image;
    gsize n_bytes_frame;
    fftw_plan fft_plan_proj;
    fftw_plan fft_plan_flats;
    gfloat *flats_cimgs;
    gfloat *projs_cimgs;
};

static void ufo_task_interface_init (UfoTaskIface *iface);

G_DEFINE_TYPE_WITH_CODE (UfoGrProjectionsProcessTask, ufo_gr_projections_process_task, UFO_TYPE_TASK_NODE,
                         G_IMPLEMENT_INTERFACE (UFO_TYPE_TASK,
                                                ufo_task_interface_init))

#define UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UFO_TYPE_GR_PROJECTIONS_PROCESS_TASK, UfoGrProjectionsProcessTaskPrivate))

enum {
    PROP_0,
    PROP_FIX_NAN_AND_INF,
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
    UfoRequisition flats_req;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (task);

    ufo_buffer_get_requisition (inputs[0], &flats_req);
    ufo_buffer_get_requisition (inputs[1], &in_req);

    requisition->n_dims = 3;
    requisition->dims[0] = flats_req.dims[0];
    requisition->dims[1] = flats_req.dims[1];
    requisition->dims[2] = 3; // dpc, dfi, aps

    priv->n_pixels_image = requisition->dims[0] * requisition->dims[1];
    priv->n_flat_fields = flats_req.dims[2];
    priv->n_bytes_frame = sizeof(gfloat) * priv->n_pixels_image;
}

static guint
ufo_gr_projections_process_task_get_num_inputs (UfoTask *task)
{
    return 2;
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

static gsize
get_max_complex (fftw_complex *arr, guint size) {
    gsize index = -1;
    gfloat max_abs, max_phase;
    gfloat tmp_abs, tmp_phase;

    max_abs = FLT_MIN;
    max_phase = FLT_MIN;

    for (gsize i = 1; i <= size/2; ++i) {
        tmp_abs = fabs(arr[i][0]);

        if (max_abs == tmp_abs) {
            tmp_phase = atan2f(arr[i][1], arr[i][0]);

            if (max_phase < tmp_phase) {
                max_phase = tmp_phase;
                index = i;
            }
        }
        else {
            if (max_abs < tmp_abs) {
                max_abs = tmp_abs;
                index = i;
            }
        }
    }

    return index;
}

static fftw_complex *
transpose_data (gfloat *data, gsize image_size, guint n_images)
{
    fftw_complex *out_data;

    out_data = fftw_malloc(sizeof(fftw_complex) * image_size * n_images);
    for (gsize i = 0; i < image_size; ++i) {
        for (gsize j = 0; j < n_images; j++) {
            out_data[i * n_images + j][0] = data[image_size * j + i];
            out_data[i * n_images + j][1] = 0.0f;
        }
    }

    return out_data;
}

static fftw_complex *
gather_image (fftw_complex *data, gsize image_size, guint n_images, guint index)
{
    fftw_complex *out_data;

    out_data = fftw_malloc(sizeof(fftw_complex) * image_size);

    for (gsize i = 0; i < image_size; ++i) {
        out_data[i][0] = data[i * n_images + index][0];
        out_data[i][1] = data[i * n_images + index][1];
    }

    return out_data;
}

static gfloat *
get_dpc_abs_dfi (fftw_complex *data, gsize image_size, guint n_images, guint n_periods)
{
    gfloat *out_data;

    out_data = (gfloat *)g_malloc0(sizeof(gfloat) * image_size * 3);

    //gather images data
    fftw_complex *data_period_tmp = gather_image(data, image_size, n_images, n_periods);
    fftw_complex *data_zero_tmp = gather_image(data, image_size, n_images, 0);

    //get dpc, abs and dfi
    for (gsize i = 0; i < image_size; ++i) {
        out_data[i] = atan2f(data_period_tmp[i][1], data_period_tmp[i][0]);
        out_data[i + image_size] = fabs(data_zero_tmp[i][0]);
        out_data[i + image_size * 2] = fabs(data_period_tmp[i][0]);
    }

    return out_data;
}

static gfloat *
correct_contrast_images (gfloat *flats_ci, gfloat *projs_ci, gsize image_size) 
{
    gfloat *out_data;

    out_data = (gfloat *)g_malloc0(sizeof(gfloat) * image_size * 3);

    for (gsize i = 0; i < image_size; ++i) {
        out_data[i] = projs_ci[i] - flats_ci[i];
        out_data[i + image_size] = projs_ci[i + image_size] / flats_ci[i + image_size];
        out_data[i + image_size * 2] = (projs_ci[i + image_size * 2] / flats_ci[i + image_size * 2]) 
                                                                            / out_data[i + image_size];
    }

    return out_data;
}

static void 
quick_sort (gfloat *a, gsize n) {
    if (n < 2)
        return;
    gfloat p = a[n / 2];
    gfloat *l = a;
    gfloat *r = a + n - 1;
    while (l <= r) {
        if (*l < p) {
            l++;
        }
        else if (*r > p) {
            r--;
        }
        else {
            gfloat t = *l;
            *l = *r;
            *r = t;
            l++;
            r--;
        }
    }
    quick_sort(a, r - a + 1);
    quick_sort(l, a + n - l);
}

static gfloat
median (gfloat *vector, gsize size) {
    return size % 2 == 0 ? (vector[size / 2 - 1] + vector[size / 2]) / 2 : vector[size / 2];
}

static gfloat
median2d (gfloat *array, gsize width, gsize height) 
{
    gfloat median_val;
    gfloat *h_vector;

    h_vector = (gfloat *)g_malloc0(sizeof(gfloat) * height);
/*
    printf("\n");
    for (gsize i = 0; i < height; i++) {
        for (gsize j = 0; j < width; j++) {
            printf("%f  ", array[i * width + j]);
        }
        printf("\n");
    }
*/
    for (gsize i = 0; i < height; i++) {
        gsize offset = i * width;
        quick_sort(array + offset, width);
        h_vector[i] = median(array + offset, width);
    }
/*
    printf("\n");
    for (gsize i = 0; i < height; i++) {
        for (gsize j = 0; j < width; j++) {
            printf("%f  ", array[i * width + j]);
        }
        printf("\n");
    }

    printf("\n");
    for (gsize i = 0; i < height; i++) {
        printf("%f  ", h_vector[i]);
    }
    printf("\n");
*/
    quick_sort(h_vector, height);
    median_val = median(h_vector, height);

    return median_val;
}

static void
differencial_filtering(gfloat *corrected_images, gsize width, gsize height) 
{
    gfloat median_val;

    median_val = median2d(corrected_images, width, height);

    for (gsize i = 0; i < width * height; ++i) {
        gfloat tmp_val = corrected_images[i] - median_val;
        corrected_images[i] = median_val - 2 * G_PI * floor((tmp_val + G_PI) / (2 * G_PI));
    }
}

static void
filter_inf_nans(gfloat *data, gsize size) 
{
    for (gsize i = 0; i < size; i++) {
        if (isnan (data[i]) || isinf (data[i])) {
            data[i] = 0.0f;
        }
    }
}

static gboolean
ufo_gr_projections_process_task_process (UfoTask *task,
                                UfoBuffer **inputs,
                                UfoBuffer *output,
                                UfoRequisition *requisition)
{

    UfoGrProjectionsProcessTaskPrivate *priv;
    UfoProfiler *profiler;
    fftw_plan fft_plan_index;
    gfloat *proj_data;
    gfloat *flat_dark_data;
    gfloat *corrected_flat_data;
    gfloat corrected_value;
    fftw_complex *flat_fields_mean;
    fftw_complex *transposed_flats;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (task);
    profiler = ufo_task_node_get_profiler (UFO_TASK_NODE (task));

    
    //ufo_profiler_start (profiler, UFO_PROFILER_TIMER_CPU);

    // PROC F0  C = 1
    // PROC F1  C = 2
    // PROC F2  C = 3
    // PROC F3  C = 4
    // GEN
    // PROC F4  C = 1
    // PROC F5  C = 2
    // PROC F5  C = 3
    // PROC F6  C = 4
    // GEN
    // PROC F7  C = 1
    // PROC F8  C = 2


    //
    // PROCESSIGN
    //

    priv->counter++;
    printf("ufo_gr_projections_process_task_process [counter = %d , gr-step = %d ]\n", priv->counter, priv->grating_step);
    if (priv->counter >= priv->grating_step) {
        g_print ("\n KENNNY!");
        priv->counter = 0;
        return FALSE;
    }
    

#if 0
    gfloat arr[] = {6.2,13.2,14.2,7.2,8.2,9.2,1.2,2.2,3.2,4.2,5.2,11.2,12.2,15.2,16.2,25.6};
    gsize width = 4;
    gsize height = 4;

    for (int i = 0; i < 16; ++i)
    {
        printf("%f ", arr[i]);
    }

    printf("\n");

    test1(arr);

    for (int i = 0; i < 16; ++i)
    {
        printf("%f ", arr[i]);
    }
#endif

#if 1
    if (priv->flats_cimgs == NULL || priv->averaged_dark_fields_data == NULL) {
        flat_dark_data = ufo_buffer_get_host_array (inputs[0], NULL);

        priv->averaged_dark_fields_data = (gfloat *) g_malloc0(priv->n_bytes_frame);
        memcpy(priv->averaged_dark_fields_data, flat_dark_data, priv->n_bytes_frame);

        corrected_flat_data = (gfloat *) g_malloc0(priv->n_bytes_frame * priv->n_flat_fields);
        memcpy(corrected_flat_data, flat_dark_data + priv->n_pixels_image, 
               priv->n_bytes_frame * priv->n_flat_fields);

        printf("---1\n");

        // Calculate number of periods and visibility
        flat_fields_mean = fftw_malloc (sizeof(fftw_complex) * priv->n_flat_fields);
printf("---2\n");
        for (gsize i = 0; i < priv->n_flat_fields; i++) {
            for (gsize j = 0; j < priv->n_pixels_image; j++) {
                flat_fields_mean[i][0] = corrected_flat_data[i * priv->n_pixels_image + j] / ((gfloat)priv->n_pixels_image);
                flat_fields_mean[i][1] = 0.0f;
            }
        }
printf("---3\n");
        fft_plan_index = fftw_plan_dft_1d (priv->n_flat_fields, flat_fields_mean, flat_fields_mean, FFTW_FORWARD, FFTW_ESTIMATE);
printf("---4\n");
        fftw_execute (fft_plan_index);
        printf("---5\n");
        printf("---6\n");

        if (priv->n_periods == -1) {
            priv->n_periods = get_max_complex(flat_fields_mean, priv->n_flat_fields);
            priv->n_periods = priv->n_periods + 1;
        }
printf("---7\n");
        // Calculate contrast images for flats
        transposed_flats = transpose_data(corrected_flat_data, priv->n_pixels_image, priv->n_flat_fields);
printf("---8\n");
        if (priv->fft_plan_flats == NULL) {
            int n[] = {priv->n_flat_fields};
            int rank = 1;
            int howmany = priv->n_pixels_image;
            int idist = n[0];
            int odist = n[0];
            int istride = 1;
            int ostride = 1;
            int *inembed = n;
            int *onembed = n;

            priv->fft_plan_flats = fftw_plan_many_dft(1, &priv->n_flat_fields, priv->n_pixels_image,
                                                      transposed_flats, &priv->n_flat_fields, 1, priv->n_flat_fields,
                                                      transposed_flats, &priv->n_flat_fields, 1, priv->n_flat_fields,
                                                      FFTW_FORWARD, FFTW_ESTIMATE);
            priv->fft_plan_flats = fftw_plan_many_dft(rank, n, howmany,
                                  transposed_flats, inembed, istride, idist,
                                  transposed_flats, onembed, ostride, odist,
                                  FFTW_FORWARD, FFTW_ESTIMATE);
        }
printf("---9\n");
        fftw_execute (priv->fft_plan_flats);
printf("---10\n");
        priv->flats_cimgs = get_dpc_abs_dfi (transposed_flats, priv->n_pixels_image, 
                                             priv->n_flat_fields, priv->n_periods);

        fftw_destroy_plan (fft_plan_index);
        g_free (corrected_flat_data);
        fftw_free(transposed_flats);
        fftw_free(flat_fields_mean);
printf("---11\n");
    }
#endif

#if 1
    proj_data = ufo_buffer_get_host_array (inputs[1], NULL);

    if (priv->projections_data == NULL) {
        priv->projections_data = (gfloat *) g_malloc0(priv->n_bytes_frame * priv->grating_step);
    }

    for (gsize i = 0; i < priv->n_pixels_image; i++) {
        corrected_value = proj_data[i] - priv->averaged_dark_fields_data[i];

        if (priv->fix_nan_and_inf && (isnan (corrected_value) || isinf (corrected_value))) {
            corrected_value = 0.0;
        }

        priv->projections_data[priv->counter * priv->n_pixels_image + i] = corrected_value;
    }

    ufo_profiler_stop (profiler, UFO_PROFILER_TIMER_CPU);
#endif
    
    g_print ("\nufo_gr_projections_process_task  %p: return TRUE\n", task);
    return TRUE;
}

static gboolean
ufo_gr_projections_process_task_generate (UfoTask *task,
                                          UfoBuffer *output,
                                          UfoRequisition *requisition)
{


    UfoGrProjectionsProcessTaskPrivate *priv;
    gfloat *out_data;
    gfloat *corrected_proj_data;
    fftw_complex *transposed_projs;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (task);

    //if (!priv->n_outs) {
    //    priv->n_outs = 1;
    //    return FALSE;
    //}
#if 1
    out_data = ufo_buffer_get_host_array (output, NULL);


    // Calculating contrast images for projections
    transposed_projs = transpose_data(priv->projections_data, priv->n_pixels_image, priv->grating_step);

    if (priv->fft_plan_proj == NULL) {
        int rank = 1;
        int n[] = {priv->grating_step};
        int batches = priv->n_pixels_image;
        int idist = n[0];
        int odist = n[0];
        int istride = 1;
        int ostride = 1;
        int *inembed = n;
        int *onembed = n;
        
        priv->fft_plan_proj = fftw_plan_many_dft(rank, n, batches,
                                                 transposed_projs, inembed, istride, idist,
                                                 transposed_projs, onembed, ostride, odist,
                                                 FFTW_FORWARD, FFTW_ESTIMATE);
    }

    fftw_execute (priv->fft_plan_proj);

    priv->projs_cimgs = get_dpc_abs_dfi (transposed_projs, priv->n_pixels_image, 
                                         priv->grating_step, priv->n_periods);

    corrected_proj_data = correct_contrast_images (priv->flats_cimgs, priv->projs_cimgs, priv->n_pixels_image);

    differencial_filtering(corrected_proj_data, requisition->dims[0], requisition->dims[1]);
    filter_inf_nans(corrected_proj_data, priv->n_pixels_image * priv->grating_step);

    memcpy(out_data, corrected_proj_data, sizeof(gfloat) * priv->n_pixels_image * priv->grating_step);

    fftw_free(transposed_projs);
    g_free(corrected_proj_data);
#endif
    printf("ufo_gr_projections_process_task_generate [GENERATE STUFF]\n");

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

    UfoGrProjectionsProcessTaskPrivate *priv;

    priv = UFO_GR_PROJECTIONS_PROCESS_TASK_GET_PRIVATE (object);
printf("---1\n");
    if (priv->averaged_dark_fields_data) {
        g_free (priv->averaged_dark_fields_data);
        priv->averaged_dark_fields_data = NULL;
    }
printf("---2\n");
    if (priv->projections_data) {
        g_free (priv->projections_data);
        priv->projections_data = NULL;
    }
printf("---3\n");
    if (priv->flats_cimgs) {
        g_free (priv->flats_cimgs);
        priv->flats_cimgs = NULL;
    }
printf("---4\n");
    if (priv->projs_cimgs) {
        g_free (priv->projs_cimgs);
        priv->projs_cimgs = NULL;
    }
printf("---5\n");
    if (priv->fft_plan_proj) {
        fftw_destroy_plan (priv->fft_plan_proj);
    }
printf("---6\n");
    if (priv->fft_plan_flats) {
        fftw_destroy_plan (priv->fft_plan_flats);
    }
printf("---7\n");
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
    self->priv->projections_data = NULL;
    self->priv->averaged_dark_fields_data = NULL;
    self->priv->fix_nan_and_inf = FALSE;
    self->priv->grating_step = 4;
    self->priv->n_flat_fields = 1;
    self->priv->fft_plan_proj = NULL;
    self->priv->fft_plan_flats = NULL;
    self->priv->flats_cimgs = NULL;
    self->priv->counter = 0;
    self->priv->n_outs = 1;
}
