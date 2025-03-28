/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#include <stdlib.h>

#include "vsi_nn_pre_post_process.h"
#include "vsi_nn_graph.h"
#include "vsi_nn_log.h"
#include "vsi_nn_test.h"

static void _create_multi_norm_tensors
    (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_attr_t* input_attr,
    vsi_nn_preprocess_source_layout_e* source_layout,
    vsi_nn_preprocess_source_format_e* source_format,
    vsi_nn_tensor_id_t* multi_input_tensors
    )
{
    vsi_size_t w = 0;
    vsi_size_t h = 0;
    uint32_t i = 0;
    vsi_nn_tensor_attr_t y_input_attr;
    vsi_nn_tensor_attr_t uv_input_attr;
    vsi_nn_tensor_attr_t rgb888_planar_sep_attr;

    if (*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
    {
        w = input_attr->size[1];
        h = input_attr->size[2];
    }
    else
    {
        w = input_attr->size[0];
        h = input_attr->size[1];
    }

    if(*source_format ==  VSI_NN_SOURCE_FORMAT_IMAGE_RGB888_PLANAR_SEP)
    {
        rgb888_planar_sep_attr = *input_attr;
        rgb888_planar_sep_attr.size[0] = w;
        rgb888_planar_sep_attr.size[1] = h;
        rgb888_planar_sep_attr.size[2] = 1;  /* channel */
        for (i = 0; i < 3; i++)
        {
            multi_input_tensors[i] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &rgb888_planar_sep_attr, NULL);
        }
    }
    else
    {
        /* Create y norm tensor */
        y_input_attr = *input_attr;
        y_input_attr.size[0] = w;
        y_input_attr.size[1] = h;
        y_input_attr.size[2] = 1;
        y_input_attr.size[3] = 1;
        multi_input_tensors[0] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &y_input_attr, NULL);

        /* Create uv norm tensor */
        if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV420)
        {
            uv_input_attr = *input_attr;
            uv_input_attr.size[0] = w / 2;
            uv_input_attr.size[1] = h / 2;
            uv_input_attr.size[2] = 1;
            uv_input_attr.size[3] = 1;

            multi_input_tensors[1] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &uv_input_attr, NULL);
            multi_input_tensors[2] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &uv_input_attr, NULL);
        }
        else if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12 ||
                 *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21 ||
                 *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12_RGGB ||
                 *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21_BGGR)
        {
            uv_input_attr = *input_attr;
            uv_input_attr.size[0] = w;
            uv_input_attr.size[1] = h / 2;
            uv_input_attr.size[2] = 1;
            uv_input_attr.size[3] = 1;

            multi_input_tensors[1] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &uv_input_attr, NULL);
        }
        else if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV444)
        {
            uv_input_attr = *input_attr;
            uv_input_attr.size[0] = w;
            uv_input_attr.size[1] = h;
            uv_input_attr.size[2] = 1;
            uv_input_attr.size[3] = 1;
            multi_input_tensors[1] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &uv_input_attr, NULL);
            multi_input_tensors[2] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &uv_input_attr, NULL);
        }
    }
} /* _create_yuv_norm_tensors() */

static vsi_status _set_preproc_node_type
    (
    vsi_nn_node_t* node,
    vsi_nn_preprocess_source_format_e* source_format
    )
{
    vsi_status status = VSI_SUCCESS;
    if(source_format != NULL)
        node->nn_param.pre_process.type = *source_format;
    else
    {
        VSILOGE("Preprocess source format need to be set!");
        status = VSI_FAILURE;
    }
    return status;
} /* _set_preproc_node_type() */

static void _set_preproc_node_rect_params
    (
    vsi_nn_node_t* node,
    vsi_nn_preprocess_crop_t* crop,
    vsi_nn_preprocess_image_size_t* input_size,
    vsi_nn_preprocess_source_format_e* source_format
    )
{
    if(crop != NULL)
    {
        if(*source_format == VSI_NN_SOURCE_FORMAT_TENSOR)
        {
            VSILOGW("don not need to set crop parameter for tensor preprocess");
        }
        else
        {
            node->nn_param.pre_process.rect.left = crop->begin[0];
            node->nn_param.pre_process.rect.top = crop->begin[1];
            node->nn_param.pre_process.rect.width = crop->size[0];
            node->nn_param.pre_process.rect.height = crop->size[1];
        }
    }
    else if (*source_format != VSI_NN_SOURCE_FORMAT_TENSOR)
    {
        if(input_size == NULL)
        {
            VSILOGE("Please set image size for preprocess node");
        }
        else
        {
            node->nn_param.pre_process.rect.left = 0;
            node->nn_param.pre_process.rect.top = 0;
            node->nn_param.pre_process.rect.width = input_size->w;
            node->nn_param.pre_process.rect.height = input_size->h;
        }
    }
} /* _set_preproc_node_rect_params() */

static void _set_preproc_node_norm_params
    (
    vsi_nn_node_t* node,
    vsi_nn_preprocess_type_e type,
    void* mean_and_scale
    )
{
    int32_t i = 0;
    if(mean_and_scale != NULL)
    {
        if (type == VSI_NN_PREPROCESS_MEAN_AND_SCALE)
        {
            vsi_nn_preprocess_mean_and_scale_t* means_and_single_scale =
            (vsi_nn_preprocess_mean_and_scale_t*)mean_and_scale;
            node->nn_param.pre_process.norm.scale = means_and_single_scale->scale;
            node->nn_param.pre_process.norm2.scale[0] = means_and_single_scale->scale;
            node->nn_param.pre_process.norm2.scale[1] = means_and_single_scale->scale;
            node->nn_param.pre_process.norm2.scale[2] = means_and_single_scale->scale;
            for (i = 0; i < means_and_single_scale->channel_len; i++)
            {
                node->nn_param.pre_process.norm.mean[i] = means_and_single_scale->channel_mean[i];
            }
        }
        else if (type == VSI_NN_PREPROCESS_MEANS_AND_SCALES)
        {
            vsi_nn_preprocess_means_and_scales_t* means_and_scales =
            (vsi_nn_preprocess_means_and_scales_t*)mean_and_scale;
            for (i = 0; i < means_and_scales->scale_len; i++)
            {
                node->nn_param.pre_process.norm2.scale[i] = means_and_scales->scale[i];
            }
            for(i = 0; i < means_and_scales->channel_len; i++)
            {
                node->nn_param.pre_process.norm.mean[i] = means_and_scales->channel_mean[i];
            }
        }
    }
    else
    {
        for(i = 0; i < 3; i++)
        {
            node->nn_param.pre_process.norm.mean[i] = 0;
            node->nn_param.pre_process.norm2.scale[i] = 1.0f;
        }
    }
} /* _set_preproc_node_norm_params() */

static void _set_preproc_node_out_attr
    (
    vsi_nn_node_t* node,
    vsi_nn_preprocess_image_resize_t* image_resize,
    vsi_nn_tensor_attr_t* attr,
    vsi_nn_preprocess_source_layout_e* source_layout
    )
{
    node->nn_param.pre_process.dim_num = attr->dim_num;
    node->nn_param.pre_process.output_attr.dim_num = attr->dim_num;
    node->nn_param.pre_process.output_attr.size = attr->size;
    if(image_resize != NULL)
    {
        node->nn_param.pre_process.output_attr.size[0]  = image_resize->w;
        node->nn_param.pre_process.output_attr.size[1]  = image_resize->h;
        node->nn_param.pre_process.output_attr.size[2]  = image_resize->c;
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
        {
            node->nn_param.pre_process.output_attr.size[0]  = image_resize->c;
            node->nn_param.pre_process.output_attr.size[1]  = image_resize->w;
            node->nn_param.pre_process.output_attr.size[2]  = image_resize->h;
        }
    }
} /* _set_preproc_node_out_attr() */

static void _set_preproc_node_input_attr
    (
    vsi_nn_tensor_attr_t* input_attr,
    vsi_nn_tensor_attr_t* attr,
    vsi_nn_preprocess_image_size_t* input_size,
    vsi_nn_preprocess_source_format_e* source_format,
    vsi_nn_preprocess_source_layout_e* source_layout
    )
{
    *input_attr = *attr;
    input_attr->dim_num = attr->dim_num;
    if(input_size != NULL)
    {
        input_attr->size[0] = input_size->w;
        input_attr->size[1] = input_size->h;
        input_attr->size[2] = input_size->c;
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
        {
            input_attr->size[0] = input_size->c;
            input_attr->size[1] = input_size->w;
            input_attr->size[2] = input_size->h;
        }
    }
    if(*source_format == VSI_NN_SOURCE_FORMAT_TENSOR)
    {
        input_attr->dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
        input_attr->dtype.vx_type = VSI_NN_TYPE_FLOAT32;
    }
    else
    {
        input_attr->dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
        input_attr->dtype.vx_type = VSI_NN_TYPE_UINT8;
    }
    if(*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_RGB)
    {
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
        {
            input_attr->size[0] = input_attr->size[1]*input_attr->size[0];
            input_attr->size[1] = input_attr->size[2];
            input_attr->size[2] = 1;
        }
        else
        {
            input_attr->size[0] = input_attr->size[2]*input_attr->size[0];
            input_attr->size[2] = 1;
        }
    }

    if(*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_RGB888_PLANAR || *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_GRAY)
    {
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC && input_size != NULL)
        {
            input_attr->size[0] = input_size->w;
            input_attr->size[1] = input_size->h;
            input_attr->size[2] = input_size->c;
        }
    }

    if(*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_BGRA)
    {
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
        {
            input_attr->size[0] = 4*input_attr->size[1];
            input_attr->size[1] = input_attr->size[2];
            input_attr->size[2] = 1;
        }
        else
        {
            input_attr->size[0] = 4*input_attr->size[0];
            input_attr->size[2] = 1;
        }
    }
    if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUYV422 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_UYVY422)
    {
        if(*source_layout == VSI_NN_SOURCE_LAYOUT_NHWC)
        {
            input_attr->size[0] = 2*input_attr->size[1];
            input_attr->size[1] = input_attr->size[2];
            input_attr->size[2] = 1;
        }
        else
        {
            input_attr->size[0] = 2*input_attr->size[0];
            input_attr->size[2] = 1;
        }
    }
} /*_set_preproc_node_input_attr() */

static void _set_preproc_node_output_attr
    (
    vsi_nn_tensor_attr_t* output_attr,
    vsi_nn_tensor_attr_t* attr,
    vsi_nn_preprocess_dtype_convert_t* data_convert
    )
{
    *output_attr = *attr;
    if(data_convert != NULL)
    {
        output_attr->dtype = data_convert->dtype;
    }
    output_attr->dtype.fmt = VSI_NN_DIM_FMT_NCHW;
    output_attr->dim_num = VSI_NN_DIM_AUTO;
    output_attr->is_const = FALSE;
    output_attr->vtl = TRUE;
} /* _set_preproc_node_output_attr() */

static void _set_postproc_node_output_attr
    (
    vsi_nn_tensor_attr_t* output_attr,
    vsi_nn_tensor_attr_t* attr,
    vsi_nn_postprocess_permute_t* permute,
    vsi_nn_postprocess_dtype_convert_t* dtype_convert
    )
{
    int32_t i = 0;
    output_attr->dim_num = attr->dim_num;
    output_attr->is_const = FALSE;
    output_attr->vtl = FALSE;
    if(dtype_convert != NULL)
    {
        output_attr->dtype = dtype_convert->dtype;
    }
    else
    {
        output_attr->dtype = attr->dtype;
    }
    if(permute != NULL)
    {
        for(i = 0; i < permute->dim; i++)
        {
            output_attr->size[i] = attr->size[permute->perm[i]];
        }
    }
    else
    {
        for(i = 0; i < (int32_t)attr->dim_num; i++)
        {
            output_attr->size[i] = attr->size[i];
        }
    }
} /* _set_postproc_node_output_attr() */

static void _reconnect_graph_inputs
    (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_id_t org_input,
    uint32_t input_idx,
    vsi_nn_tensor_id_t* inputs,
    uint32_t inputs_num
    )
{
    vsi_nn_tensor_id_t cur_input;
    uint32_t i;
    uint32_t final_idx;

    final_idx = input_idx;
    /* get the new input idx */
    for(i = input_idx; i < graph->input.num; i++)
    {
        cur_input = graph->input.tensors[i];
        if(cur_input == org_input)
        {
            final_idx = i;
            break;
        }
    }
    /* move next inputs to save space for new inputs*/
    for(i = graph->input.num-1; i > final_idx + inputs_num - 1; i--)
    {
        graph->input.tensors[i] = graph->input.tensors[i - inputs_num + 1];
    }

    /* connect new inputs */
    for(i = 0; i < inputs_num; i++)
    {
        graph->input.tensors[final_idx + i] = inputs[i];
    }
}/* _reconnect_graph_inputs() */

static void _get_org_graph_inputs
    (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_id_t* graph_inputs
    )
{
    uint32_t i;
    uint32_t idx = 0;
    vsi_nn_tensor_id_t cur_input;
    uint32_t nodes_count = 0;
    vsi_nn_node_t* nodes[1] = {NULL};
    for(i = 0; i < graph->input.num; i++)
    {
        cur_input = graph->input.tensors[i];
        vsi_nn_get_tensor_consumers(graph, cur_input, NULL, &nodes_count);
        if(nodes_count == 1)
        {
            vsi_nn_get_tensor_consumers(graph, cur_input, nodes, NULL);
            if(nodes[0]->op == VSI_NN_OP_PRE_PROCESS)
            {
                if(nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_YUV420 ||
                   nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_YUV444 )
                {
                    i += 2 ;
                }
                else if(nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_NV12 ||
                        nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_NV21 ||
                        nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_NV12_RGGB ||
                        nodes[0]->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_NV21_BGGR)
                {
                    i += 1;
                }
            }
        }

        graph_inputs[idx] = cur_input;
        idx += 1;
    }
}/* _get_org_graph_inputs() */

vsi_status vsi_nn_add_single_preproc_node
    (
    vsi_nn_graph_t* graph,
    uint32_t input_idx,
    vsi_nn_tensor_id_t org_input,
    vsi_nn_node_t** first_node,
    uint32_t nodes_count,
    vsi_nn_preprocess_base_t* preprocess,
    uint32_t proc_count
    )
{
    vsi_status status = VSI_FAILURE;
    vsi_nn_preprocess_source_format_e* source_format = NULL;
    vsi_nn_preprocess_source_layout_e* source_layout = NULL;
    vsi_nn_node_t* node = NULL;
    vsi_nn_preprocess_image_size_t* input_size = NULL;
    vsi_nn_preprocess_crop_t* crop = NULL;
    void* mean_and_scale = NULL;
    vsi_nn_preprocess_permute_t* permute = NULL;
    vsi_nn_preprocess_image_resize_t* image_resize = NULL;
    vsi_nn_preprocess_dtype_convert_t* data_convert = NULL;
    vsi_nn_tensor_attr_t  input_attr;
    vsi_nn_tensor_attr_t  output_attr;
    vsi_nn_tensor_id_t preproc_inputs[3] = {0};
    vsi_nn_tensor_id_t preproc_output;
    vsi_nn_tensor_t* org_norm_tensor = NULL;
    vsi_nn_preprocess_type_e mean_and_scale_type = VSI_NN_PREPROCESS_MEAN_AND_SCALE;
    uint32_t node_input_num = 1;
    int32_t reverse_channel = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t idx =0;

    org_norm_tensor = vsi_nn_GetTensor(graph, org_input);
    TEST_CHECK_PTR(org_norm_tensor, final);

    /* Get preprocess configurations*/
    for(idx = 0; idx < proc_count; idx++)
    {
       if(preprocess[idx].type == VSI_NN_PREPROCESS_SOURCE_LAYOUT)
           source_layout = (vsi_nn_preprocess_source_layout_e*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_SET_SOURCE_FORMAT)
           source_format = (vsi_nn_preprocess_source_format_e*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_CROP)
           crop = (vsi_nn_preprocess_crop_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_MEAN_AND_SCALE)
           mean_and_scale = (vsi_nn_process_mean_and_scale_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_PERMUTE)
           permute = (vsi_nn_process_permute_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_IMAGE_RESIZE_BILINEAR||
               preprocess[idx].type == VSI_NN_PREPROCESS_IMAGE_RESIZE_NEAREST)
           image_resize = (vsi_nn_preprocess_image_resize_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_REVERSE_CHANNEL)
           reverse_channel = *(uint8_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_DTYPE_CONVERT)
           data_convert = (vsi_nn_preprocess_dtype_convert_t*)preprocess[idx].param;

       else if(preprocess[idx].type == VSI_NN_PREPROCESS_IMAGE_SIZE)
           input_size = (vsi_nn_preprocess_image_size_t*)preprocess[idx].param;
       else if(preprocess[idx].type == VSI_NN_PREPROCESS_MEANS_AND_SCALES)
       {
           mean_and_scale = (vsi_nn_process_means_and_scales_t*)preprocess[idx].param;
           mean_and_scale_type  = VSI_NN_PREPROCESS_MEANS_AND_SCALES;
       }
       else
       {
           VSILOGE("preprocess[%d] type is not support, please have a check!", idx);
           status = VSI_FAILURE;
           TEST_CHECK_STATUS(status, final);
       }
    }

    if (source_layout == NULL)
    {
        VSILOGE("Preprocess source layout need to be set!");
        status = VSI_FAILURE;
        TEST_CHECK_STATUS(status, final);
    }

    if (source_format == NULL)
    {
        VSILOGE("Preprocess source source format need to be set!");
        status = VSI_FAILURE;
        TEST_CHECK_STATUS(status, final);
    }

    /* Add preprocess node */
    if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV420 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV444 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_RGB888_PLANAR_SEP)
    {
        node_input_num = 3;
    }
    else if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12 ||
             *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21 ||
             *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12_RGGB ||
             *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21_BGGR)
    {
        node_input_num = 2;
    }

    node = vsi_nn_AddNode(graph, VSI_NN_OP_PRE_PROCESS, node_input_num, 1, NULL);
    TEST_CHECK_PTR(node, final);
    node->uid = (uint32_t)(VSI_NN_PREPROC_NODE_UID_BASE) + input_idx;

    /* Set preprocess node parameters */
    status = _set_preproc_node_type(node, source_format);
    TEST_CHECK_STATUS(status, final);

    _set_preproc_node_rect_params(node, crop, input_size, source_format);
    _set_preproc_node_norm_params(node, mean_and_scale_type, mean_and_scale);

    if(permute != NULL)
    {
        if((uint32_t)permute->dim != org_norm_tensor->attr.dim_num)
        {
            VSILOGE("Preprocess permute dim dosen't match input dim");
            status = VSI_FAILURE;
            TEST_CHECK_STATUS(status, final);
        }
        node->nn_param.pre_process.perm = (uint32_t*)permute->perm;
    }

    if(reverse_channel)
    {
        node->nn_param.pre_process.reverse_channel = TRUE;
    }
    else
    {
        node->nn_param.pre_process.reverse_channel = FALSE;
    }

    _set_preproc_node_out_attr(node, image_resize, &org_norm_tensor->attr, source_layout);

    /* Set input tensor attr */
    _set_preproc_node_input_attr(&input_attr, &org_norm_tensor->attr, input_size, source_format, source_layout);

    /* Set output tensor attr */
    _set_preproc_node_output_attr(&output_attr, &org_norm_tensor->attr, data_convert);

    /* Create new norm and virtual tensors */
    if (*source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV420 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_YUV444 ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_RGB888_PLANAR_SEP ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV12_RGGB ||
        *source_format == VSI_NN_SOURCE_FORMAT_IMAGE_NV21_BGGR)
    {
        _create_multi_norm_tensors(graph, &input_attr, source_layout, source_format, preproc_inputs);
    }
    else
    {
        preproc_inputs[0] = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &input_attr, NULL);
    }
    preproc_output = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &output_attr, NULL);

    /* Reconnect node tensors */
    for(i = 0; i < nodes_count; i++)
    {
        for(j = 0; j < first_node[i]->input.num; j++)
        {
            if(first_node[i]->input.tensors[j] == org_input)
            {
                first_node[i]->input.tensors[j] = preproc_output;
                break;
            }
        }
    }

    for(i = 0; i < node_input_num; i++)
    {
        node->input.tensors[i] = preproc_inputs[i];
    }
    _reconnect_graph_inputs(graph, org_input, input_idx, preproc_inputs, node_input_num);

    node->output.tensors[0] = preproc_output;

    status = VSI_SUCCESS;

final:
    return status;
} /* vsi_nn_add_single_preproc_node() */

vsi_status vsi_nn_add_single_postproc_node
    (
    vsi_nn_graph_t* graph,
    uint32_t output_idx,
    vsi_nn_node_t* last_node,
    vsi_nn_postprocess_base_t* postprocess,
    uint32_t proc_count
    )
{
    vsi_nn_node_t* node;
    vsi_nn_node_t** consume_nodes = NULL;
    vsi_nn_process_permute_t* permute = NULL;
    vsi_nn_tensor_t* org_norm_tensor = NULL;
    vsi_nn_tensor_attr_t  input_attr;
    vsi_nn_tensor_attr_t  output_attr;
    vsi_nn_tensor_id_t postproc_input;
    vsi_nn_tensor_id_t postproc_output;
    vsi_nn_postprocess_dtype_convert_t* dtype_convert = NULL;
    uint32_t i = 0;
    uint32_t j = 0;
    int32_t idx = 0;
    uint32_t nodes_count = 0;
    vsi_status status = VSI_SUCCESS;

    org_norm_tensor = vsi_nn_GetTensor(graph, graph->output.tensors[output_idx]);
    TEST_CHECK_PTR( org_norm_tensor, final );

    /*Create postprocess node*/
    node = vsi_nn_AddNode(graph, VSI_NN_OP_POST_PROCESS, 1, 1, NULL);
    TEST_CHECK_PTR( node, final );
    node->uid = (uint32_t)(VSI_NN_POSTPROC_NODE_UID_BASE) + output_idx;

    /* Get postprocess condigurations */
    for(idx = 0; idx < (int32_t)proc_count; idx++)
    {
        if(postprocess[idx].type == VSI_NN_POSTPROCESS_PERMUTE)
            permute = (vsi_nn_process_permute_t*)postprocess[idx].param;

        if(postprocess[idx].type == VSI_NN_POSTPROCESS_DTYPE_CONVERT)
            dtype_convert = (vsi_nn_postprocess_dtype_convert_t*)postprocess[idx].param;
    }

    /* Set Postprocess node parameters */
    if(permute != NULL)
    {
        if((uint32_t)permute->dim != org_norm_tensor->attr.dim_num)
        {
            VSILOGE("Postprocess permute dim doesn't match output dim!");
            status = VSI_FAILURE;
            TEST_CHECK_STATUS(status, final);
        }
        node->nn_param.post_process.perm = (uint32_t*)permute->perm;
    }
    node->nn_param.post_process.dim_num = org_norm_tensor->attr.dim_num;

    /* Set input tensor attr */
    input_attr = org_norm_tensor->attr;
    input_attr.dim_num = VSI_NN_DIM_AUTO;
    input_attr.is_const = FALSE;
    input_attr.vtl = TRUE;

    /* Set output tensor attr */
    _set_postproc_node_output_attr(&output_attr, &org_norm_tensor->attr, permute, dtype_convert);

    /* Create new norm and virtual tensor */
    postproc_input = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &input_attr, NULL);
    postproc_output = vsi_nn_AddTensor(graph, VSI_NN_TENSOR_ID_AUTO, &output_attr, NULL);

    /* Get origin norm tensor comsume nodes and connect its' comsume nodes */
    vsi_nn_get_tensor_consumers(graph, graph->output.tensors[output_idx], NULL, &nodes_count);
    if(nodes_count != 0)
    {
        consume_nodes = (vsi_nn_node_t**)malloc(sizeof(vsi_nn_node_t*)*nodes_count);
        TEST_CHECK_PTR( consume_nodes, final );
        vsi_nn_get_tensor_consumers(graph, graph->output.tensors[output_idx], consume_nodes, NULL);
        for(i = 0; i < nodes_count; i++)
            {
                for(j = 0; j < consume_nodes[i]->input.num; j++)
                    {
                        if(consume_nodes[i]->input.tensors[j] == graph->output.tensors[output_idx])
                        {
                            consume_nodes[i]->input.tensors[j] = postproc_input;
                            break;
                        }
                    }
            }
    }

    /* Reconnect node tensors */
    if (NULL == node->input.tensors)
    {
        status = VSI_FAILURE;
        goto final;
    }
    node->input.tensors[0] = postproc_input;
    if (NULL == node->output.tensors)
    {
        status = VSI_FAILURE;
        goto final;
    }
    node->output.tensors[0] = postproc_output;
    for(i = 0; i < last_node->output.num; i++)
    {
        if(last_node->output.tensors[i] == graph->output.tensors[output_idx])
        {
            last_node->output.tensors[i] = postproc_input;
            break;
        }
    }
    graph->output.tensors[output_idx] = postproc_output;


final:
    if(consume_nodes)
    {
        free(consume_nodes);
        consume_nodes = NULL;
    }
    return status;
} /* vsi_nn_add_single_postproc_node() */

vsi_status vsi_nn_AddGraphPreProcess
    (
    vsi_nn_graph_t* graph,
    uint32_t input_idx,
    vsi_nn_preprocess_base_t* preprocess,
    uint32_t count
    )
{
    vsi_status status = VSI_FAILURE;
    vsi_nn_tensor_id_t input;
    uint32_t nodes_count = 0;
    vsi_nn_node_t** nodes = NULL;
    vsi_nn_tensor_id_t* graph_inputs=NULL;

    graph_inputs = (vsi_nn_tensor_id_t*)malloc(sizeof(vsi_nn_tensor_id_t)*graph->input.num);
    TEST_CHECK_PTR( graph_inputs, final );
    _get_org_graph_inputs(graph, graph_inputs);
    input = graph_inputs[input_idx];
    vsi_nn_get_tensor_consumers(graph, input, NULL, &nodes_count);
    if(nodes_count != 0)
    {
        nodes = (vsi_nn_node_t**)malloc(sizeof(vsi_nn_node_t*)*nodes_count);
        TEST_CHECK_PTR( nodes, final );
        vsi_nn_get_tensor_consumers(graph, input, nodes, NULL);
        status = vsi_nn_add_single_preproc_node(graph, input_idx, input, nodes, nodes_count, preprocess, count);
    }

final:
    if(nodes)
    {
        free(nodes);
        nodes = NULL;
    }
    if(graph_inputs)
    {
        free(graph_inputs);
        graph_inputs = NULL;
    }
    return status;
} /* vsi_nn_AddGraphPreProcess() */

vsi_status vsi_nn_AddGraphPostProcess
    (
    vsi_nn_graph_t* graph,
    uint32_t output_idx,
    vsi_nn_postprocess_base_t* postprocess,
    uint32_t count
    )
{
    vsi_status status = VSI_FAILURE;
    vsi_nn_tensor_id_t output;
    vsi_nn_node_t * node = NULL;

    output = graph->output.tensors[output_idx];
    vsi_nn_get_tensor_provider(graph, output, &node);
    if(node != NULL)
    {
        status = vsi_nn_add_single_postproc_node(graph, output_idx, node, postprocess, count);
    }

    return status;
} /* vsi_nn_AddGraphPostProcess() */

#define WKSP(_NODE_PTR) \
    ((vsi_nn_internal_node_wksp_t*)((_NODE_PTR)->internal_node_wksp))
vsi_status vsi_nn_AddBinaryGraphInputsWithCropParam
(
    vsi_nn_graph_t* graph,
    vsi_nn_node_id_t* enable_nodes,
    uint32_t enable_nodes_count
)
{
    vsi_bool* crop_set_start_only = NULL;
    vsi_status status = VSI_FAILURE;
    crop_set_start_only = (vsi_bool*)malloc(enable_nodes_count * sizeof(vsi_bool));
    TEST_CHECK_PTR( crop_set_start_only, final );
    memset(crop_set_start_only, 0, enable_nodes_count * sizeof(vsi_bool));
    status = vsi_nn_AddBinaryGraphInputsWithCropParamForCropOnly(graph, enable_nodes,
                                                                 crop_set_start_only, enable_nodes_count);
final:
    if(crop_set_start_only)
    {
        free(crop_set_start_only);
        crop_set_start_only = NULL;
    }
    return status;
} /* vs_nn_AddBinaryGraphInputsWithCropParam() */

vsi_status vsi_nn_AddBinaryGraphInputsWithCropParamForCropOnly
(
    vsi_nn_graph_t* graph,
    vsi_nn_node_id_t* enable_nodes,
    vsi_bool* crop_set_start_only,
    uint32_t enable_nodes_count
)
{
    uint32_t i, j, k, idx, p;
    vsi_status status = VSI_FAILURE;
    uint32_t num_of_graph_inputs;
    uint32_t num_of_graph_real_inputs;
    vx_reference* graph_inputs = NULL;
    uint32_t num_of_graph_outputs;
    uint32_t num_of_graph_real_outputs;
    vx_reference* graph_outputs = NULL;
    vsi_nn_tensor_t* tensor = NULL;
    vsi_nn_node_t** nodes = NULL;
    vsi_nn_node_t* node = NULL;
    vsi_nn_node_id_t* processed_node_id_list = NULL;
    uint32_t processed_idx = 0;

    num_of_graph_real_inputs = 0;
    num_of_graph_real_outputs = 0;

    /* Explicitly set graph inputs and outputs */
    num_of_graph_inputs = graph->input.num;
    processed_node_id_list = (vsi_nn_node_id_t*)malloc(num_of_graph_inputs * sizeof(vsi_nn_node_id_t));
    TEST_CHECK_PTR( processed_node_id_list, final );
    memset(processed_node_id_list, 0, num_of_graph_inputs * sizeof(vsi_nn_node_id_t));
    processed_idx = 0;
    for (i = 0; i < num_of_graph_inputs; i++)
    {
        vsi_bool processed = FALSE;
        vsi_bool enabled = FALSE;
        uint32_t nodes_count = 0;
        num_of_graph_real_inputs += 1;
        tensor = vsi_nn_GetTensor(graph, graph->input.tensors[i]);
        nodes = NULL;
        vsi_nn_get_tensor_consumers(graph, graph->input.tensors[i], NULL, &nodes_count);
        if (nodes_count != 0)
        {
            nodes = (vsi_nn_node_t**)malloc(sizeof(vsi_nn_node_t*) * nodes_count);
            TEST_CHECK_PTR( nodes, final );
            vsi_nn_get_tensor_consumers(graph, graph->input.tensors[i], nodes, NULL);
            for (j = 0; j < nodes_count; j++)
            {
                node = nodes[j];
                for (k = 0; k < num_of_graph_inputs; k++)
                {
                    if (node->uid == processed_node_id_list[k])
                    {
                        processed = TRUE;
                        break;
                    }
                }
                for (k = 0; k < enable_nodes_count; k++)
                {
                    if (node->uid == enable_nodes[k])
                    {
                        enabled = TRUE;
                        break;
                    }
                }
                if (!processed && enabled)
                {
                    processed_node_id_list[processed_idx++] = node->uid;
                    if (node->op == VSI_NN_OP_PRE_PROCESS && node->nn_param.pre_process.type !=
                            VSI_NN_SOURCE_FORMAT_TENSOR)
                    {
                        //if(node->nn_param.pre_process.type == VSI_NN_SOURCE_FORMAT_IMAGE_RGB888_PLANAR)
                        //{
                            /* 2 additional input tensors and 4 paramter scalar*/
                        //    num_of_graph_real_inputs += 6;
                        //}
                        //else
                        //{
                        if (crop_set_start_only[j])
                        {
                            num_of_graph_real_inputs += 2;
                        }
                        else
                        {
                            num_of_graph_real_inputs += 4;
                        }
                        //}
                    }
                }
            }
            vsi_nn_safe_free(nodes);
        }
    }

    graph_inputs = (vx_reference*)malloc(num_of_graph_real_inputs * sizeof(vx_reference));
    TEST_CHECK_PTR( graph_inputs, final );
    memset(graph_inputs,  0, num_of_graph_inputs * sizeof(vx_reference));
    memset(processed_node_id_list,  0, num_of_graph_inputs * sizeof(vsi_nn_node_id_t));
    processed_idx = 0;
    for (i = 0, j=0; i < num_of_graph_inputs; i++)
    {
        vsi_bool processed = FALSE;
        vsi_bool enabled = FALSE;
        uint32_t nodes_count = 0;
        tensor = vsi_nn_GetTensor(graph, graph->input.tensors[i]);
        TEST_CHECK_PTR( tensor, final );
        vsi_nn_get_tensor_consumers(graph, graph->input.tensors[i], NULL, &nodes_count);
        if (nodes_count != 0)
        {
            nodes = (vsi_nn_node_t**)malloc(sizeof(vsi_nn_node_t*) * nodes_count);
            TEST_CHECK_PTR( nodes, final );
            vsi_nn_get_tensor_consumers(graph, graph->input.tensors[i], nodes, NULL);
            for (k = 0; k < nodes_count; k++)
            {
                node = nodes[k];
                for (idx = 0; idx < num_of_graph_inputs; idx++)
                {
                    if (node->uid == processed_node_id_list[idx])
                    {
                        processed = TRUE;
                        break;
                    }
                }
                for (idx = 0; idx < enable_nodes_count; idx++)
                {
                    if (node->uid == enable_nodes[idx])
                    {
                        enabled = TRUE;
                        break;
                    }
                }
                if (!processed)
                {
                    processed_node_id_list[processed_idx++] = node->uid;
                    if (enabled)
                    {
                        vx_node prenode = NULL;
                        vx_uint32 numParams = 0;
                        vsi_nn_internal_node_t* curr = NULL;
                        curr = WKSP(node)->nodes;
                        while (NULL != curr)
                        {
                            if (curr->node->op != VSI_NN_OP_PRE_PROCESS_TENSOR)
                            {
                                int scalar_index = 0;
                                numParams = 0;
                                prenode = curr->node->n;
                                status = vxQueryNode(prenode,
                                                     VX_NODE_PARAMETERS,
                                                     &numParams,
                                                     sizeof(numParams));
                                if (VSI_SUCCESS != status)
                                {
                                    goto final;
                                }
                                for (p = 0; p < numParams; p++)
                                {
                                    vx_parameter param = 0;
                                    vx_reference ref = 0;
                                    vx_enum type = 0;
                                    vx_enum direction = 0;
                                    vx_enum data_type = 0;

                                    param = vxGetParameterByIndex(prenode, p);
                                    if (param)
                                    {
                                        vxQueryParameter(param,
                                                         VX_PARAMETER_TYPE,
                                                         &type,
                                                         sizeof(vx_enum));
                                        vxQueryParameter(param,
                                                         VX_PARAMETER_DIRECTION,
                                                         &direction,
                                                         sizeof(vx_enum));
                                        if (direction != VX_INPUT) continue;
                                        vxQueryParameter(param,
                                                         VX_PARAMETER_REF,
                                                         &ref,
                                                         sizeof(vx_reference));
                                    }
                                    if (type == VX_TYPE_TENSOR)
                                    {
                                        graph_inputs[j++] = ref;
                                    }
                                    else if (type == VX_TYPE_SCALAR)
                                    {
                                        vxQueryScalar((vx_scalar)ref,
                                                      VX_SCALAR_TYPE,
                                                      &data_type,
                                                      sizeof(vx_enum));
                                        /*corp w, h, start_x, start_y are int32 type,
                                         * and index <4 , mean and scale are float*/
                                        if (crop_set_start_only[k])
                                        {
                                            if (data_type != VX_TYPE_INT32)
                                                continue;
                                            if (scalar_index < 4 && scalar_index >=2)
                                            {
                                                graph_inputs[j++] = ref;
                                            }
                                            scalar_index++;
                                        }
                                        else
                                        {
                                            if (data_type == VX_TYPE_INT32 &&
                                                scalar_index < 4)
                                            {
                                                graph_inputs[j++] = ref;
                                                scalar_index++;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            else
                            {
                                graph_inputs[j++] = (vx_reference)(tensor->t);
                            }
                            curr = (vsi_nn_internal_node_t*)vsi_nn_LinkListNext(
                                (vsi_nn_link_list_t*)curr);
                        }
                    }
                    else
                    {
                        graph_inputs[j++] = (vx_reference)(tensor->t);
                    }
                }
            }
            vsi_nn_safe_free(nodes);
        }
    }
    num_of_graph_outputs = graph->output.num;
    if (graph->complete_signal.exists)
    {
        num_of_graph_outputs += 1;
    }
    for (i = 0; i < num_of_graph_outputs; i++)
    {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
        if (tensor)
        {
            num_of_graph_real_outputs += 1;
        }
    }
    graph_outputs = (vx_reference*)malloc(num_of_graph_real_outputs * sizeof(vx_reference));
    TEST_CHECK_PTR( graph_outputs, final );
    memset(graph_outputs,  0, num_of_graph_real_outputs * sizeof(vx_reference));
    for (i = 0, j = 0; i < num_of_graph_outputs; i++)
    {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
        if (tensor)
        {
            if (j > num_of_graph_real_outputs - 1)
            {
                status = VSI_FAILURE;
                goto final;
            }
            graph_outputs[j++] = (vx_reference)(tensor->t);
        }
    }
    if (graph->complete_signal.exists)
    {
        graph_outputs[num_of_graph_real_outputs - 1] = (vx_reference)graph->complete_signal.tensor->t;
    }

    status = vxIdentifyGraphInputsAndOutputs(graph->g,
                                             num_of_graph_real_inputs,
                                             graph_inputs,
                                             num_of_graph_real_outputs,
                                             graph_outputs);

    if (VSI_SUCCESS != status)
    {
        goto final;
    }

final:
    if (NULL != processed_node_id_list)
    {
        free(processed_node_id_list);
    }
    if (NULL != graph_inputs)
    {
        free(graph_inputs);
    }
    if (NULL != graph_outputs)
    {
        free(graph_outputs);
    }
    return status;
} /* vsi_nn_AddBinaryGraphInputsWithCropParamForCropOnly() */

vsi_status vsi_nn_UpdateCropParamsForBinaryGraph
(
    vsi_nn_graph_t* graph,
    uint32_t enabled_crop_input_idx,
    uint32_t start_x,
    uint32_t start_y,
    uint32_t crop_w,
    uint32_t crop_h,
    uint32_t dst_w,
    uint32_t dst_h
)
{
    uint32_t i, j;
    uint32_t numParams = 0;
    int32_t scalar_value[4] = {0};
    uint32_t scalar_value_idx = 0;
    vsi_status status = VSI_SUCCESS;
    uint32_t input_idx = enabled_crop_input_idx;
    uint32_t scalar_num = 0;
    scalar_value[0] = (int32_t)((crop_w << 15) / dst_w);
    scalar_value[1] = (int32_t)((crop_h << 15) / dst_h);
    scalar_value[2] = start_x; /*rgb start_x*3, rgb start_x*4*/
    scalar_value[3] = start_y;

    for (i = 0; i < graph->node_num; i++)
    {
        vsi_nn_node_t* node = vsi_nn_GetNode(graph, i);
        if (node && node->op == VSI_NN_OP_NBG)
        {
            vx_parameter param = 0;
            vx_enum type = 0;
            vx_enum direction = 0;
            vx_reference ref = 0;
            uint32_t scalar_idx = 0;
            uint32_t scalar_start_idx = 0;
            uint32_t scalar_end_idx = 0;
            int32_t temp_value = 0;
            uint32_t cur_input_index = 0;
            status |= vxQueryNode(node->n, VX_NODE_PARAMETERS, &numParams, sizeof(numParams));
            while (input_idx > 0)
            {
                for (j = cur_input_index; j < numParams; j++)
                {

                    param = vxGetParameterByIndex(node->n, j);
                    if (param)
                    {
                        status |= vxQueryParameter(param,  VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
                        if (type == VX_TYPE_SCALAR)
                        {
                            scalar_idx = j;
                            break;
                        }
                    }
                }
                for (j = scalar_idx; j < numParams; j++)
                {
                    param = vxGetParameterByIndex(node->n, j);
                    if (param)
                    {
                        status |= vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
                        status |= vxQueryParameter(param,VX_PARAMETER_DIRECTION, &direction,sizeof(vx_enum));
                        if (type == VX_TYPE_TENSOR && direction == VX_INPUT)
                        {
                            cur_input_index = j;
                            input_idx--;
                            break;
                        }
                    }
                }
            }
            for (j = cur_input_index; j < numParams; j++)
            {
                param = vxGetParameterByIndex(node->n, j);
                if(param)
                {
                    status |= vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
                    if (type == VX_TYPE_SCALAR)
                    {
                        scalar_start_idx = j;
                        break;
                    }
                }
            }
            for (j = scalar_start_idx; j < numParams; j++)
            {
                param = vxGetParameterByIndex(node->n, j);
                if (param)
                {
                    status |= vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
                    if (type == VX_TYPE_TENSOR)
                    {
                        scalar_end_idx = j - 1;
                        break;
                    }
                }
            }
            scalar_num = scalar_end_idx - scalar_start_idx + 1;
            if (scalar_num == 2)
            {
                scalar_value_idx = 2;
            }
            for (j = scalar_start_idx; j < scalar_end_idx + 1; j++)
            {
                temp_value = scalar_value[scalar_value_idx++];
                param = vxGetParameterByIndex(node->n, j);
                if (param)
                {
                    status |= vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
                    if (type == VX_TYPE_SCALAR)
                    {
                        status |= vxQueryParameter(param, VX_PARAMETER_REF, &ref, sizeof(vx_reference));
                        status |= vxWriteScalarValue((vx_scalar)ref, &temp_value);
                        status |= vxSetParameterByIndex(node->n, j, ref);
                    }
                }
            }
            vxReleaseParameter(&param);
            param = NULL;
        }
    }
    return status;
} /* vsi_nn_UpdateCropParamsForBinaryGraph() */
