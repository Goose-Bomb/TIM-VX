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
#include <string.h>
#include <stdlib.h>

#include "vsi_nn_types.h"
#include "vsi_nn_platform.h"
#include "vsi_nn_log.h"
#include "vsi_nn_graph.h"
#include "vsi_nn_node.h"
#include "vsi_nn_prv.h"
#include "utils/vsi_nn_math.h"
#include "utils/vsi_nn_util.h"
#include "vsi_nn_ops.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_tensor_util.h"
#include "vsi_nn_internal_node.h"
#include "vsi_nn_rnn_helper.h"
#include "vsi_nn_error.h"

static vsi_bool setup_op_shapes
    (
    vsi_nn_node_t * self,
    vsi_nn_tensor_t ** inputs,
    vsi_nn_tensor_t ** outputs
    )
{
    vsi_nn_bidirectional_sequence_rnn_param* curr_param =
        &self->nn_param.bidirectional_sequence_rnn;
    vsi_nn_tensor_attr_t attr;
    vsi_nn_internal_tensor_t* output_tensor = NULL;
    vsi_size_t num_units =  0;
    vsi_size_t output_size = 0;
    vsi_size_t batch_size = 0;
    vsi_bool use_virtual_tensor = TRUE;

    memset(&attr, 0, sizeof(vsi_nn_tensor_attr_t));
    if( curr_param->time_major )
    {
        batch_size = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
    }
    else
    {
        batch_size = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
    }

    num_units = inputs[BI_RNN_FW_INPUT_WEIGHT_I]->attr.size[1];
    output_size = num_units;

    /* create h_state if app doesn't provide them */
    if( !inputs[BI_RNN_FW_INPUT_H_STATE] )
    {
        attr.dim_num = 2;
        attr.size[1] = batch_size;
        attr.size[0] = output_size;
        memcpy(&attr.dtype, &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, sizeof( attr.dtype ));
        attr.vtl = FALSE;
        attr.is_const = TRUE;

        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        inputs[BI_RNN_FW_INPUT_H_STATE] = output_tensor->t;
    }

    if( !inputs[BI_RNN_BW_INPUT_H_STATE] )
    {
        attr.dim_num = 2;
        attr.size[1] = batch_size;
        attr.size[0] = output_size;
        memcpy(&attr.dtype, &outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.dtype, sizeof( attr.dtype ));
        attr.vtl = FALSE;
        attr.is_const = TRUE;

        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        inputs[BI_RNN_BW_INPUT_H_STATE] = output_tensor->t;
    }

    if( !outputs[BI_RNN_FW_OUTPUT_H_STATE] )
    {
        memset( attr.size, 0, VSI_NN_MAX_DIM_NUM * sizeof(vsi_size_t));
        attr.dim_num = VSI_NN_DIM_AUTO;
        memcpy( &attr.dtype, &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, sizeof( attr.dtype ) );
        attr.vtl = use_virtual_tensor;
        attr.is_const = FALSE;
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        outputs[BI_RNN_FW_OUTPUT_H_STATE] = output_tensor->t;
    }

    if( !outputs[BI_RNN_BW_OUTPUT_H_STATE] )
    {
        memset( attr.size, 0, VSI_NN_MAX_DIM_NUM * sizeof(vsi_size_t));
        attr.dim_num = VSI_NN_DIM_AUTO;
        memcpy( &attr.dtype, &outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.dtype, sizeof( attr.dtype ) );
        attr.vtl = use_virtual_tensor;
        attr.is_const = FALSE;
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        outputs[BI_RNN_BW_OUTPUT_H_STATE] = output_tensor->t;
    }

    /* output */
    if( VSI_NN_DIM_AUTO == outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dim_num )
    {
        if( curr_param->merge_outputs )
        {
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[0] = output_size * 2;
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[1] = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[2] = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dim_num = 3;
        }
        else
        {
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[0] = output_size;
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[1] = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.size[2] = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
            outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dim_num = 3;

            outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.size[0] = output_size;
            outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.size[1] = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
            outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.size[2] = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
            outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.dim_num = 3;
        }
    }

    /* output_state_out */
    if(VSI_NN_DIM_AUTO == outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.dim_num)
    {
        if( curr_param->merge_outputs )
        {
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.size[0] = output_size*2;
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.size[1] = batch_size;
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.dim_num = 2;
        }
        else
        {
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.size[0] = output_size;
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.size[1] = batch_size;
            outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.dim_num = 2;

            outputs[BI_RNN_BW_OUTPUT_H_STATE]->attr.size[0] = output_size;
            outputs[BI_RNN_BW_OUTPUT_H_STATE]->attr.size[1] = batch_size;
            outputs[BI_RNN_BW_OUTPUT_H_STATE]->attr.dim_num = 2;
        }
    }
    return TRUE;
final:
    return FALSE;
}

static vsi_status op_compute
    (
    vsi_nn_node_t * self,
    vsi_nn_tensor_t ** inputs,
    vsi_nn_tensor_t ** outputs
    )
{
    VSI_UNREFERENCED(inputs);
    VSI_UNREFERENCED(outputs);
    return vsi_nn_internal_compute_node( self );
} /* op_compute() */

static vsi_bool op_check
    (
    vsi_nn_node_t * self,
    vsi_nn_tensor_t ** inputs,
    vsi_nn_tensor_t ** outputs
    )
{
    VSI_UNREFERENCED(self);
    VSI_UNREFERENCED(inputs);
    VSI_UNREFERENCED(outputs);
    /*TODO: Check tensor shapes. */
    return TRUE;
} /* op_check() */

static vsi_status op_optimize
    (
    vsi_nn_node_t * self,
    vsi_nn_tensor_t ** inputs,
    vsi_nn_tensor_t ** outputs,
    vsi_nn_opt_direction_e direction
    )
{
    VSI_UNREFERENCED(inputs);
    VSI_UNREFERENCED(outputs);
    return vsi_nn_internal_optimize_node( self, direction );
} /* op_optimize() */


static vsi_bool op_setup
    (
    vsi_nn_node_t * self,
    vsi_nn_tensor_t ** inputs,
    vsi_nn_tensor_t ** outputs
    )
{
    vsi_nn_bidirectional_sequence_rnn_param* curr_param =
        &self->nn_param.bidirectional_sequence_rnn;
    vsi_nn_internal_node_t* curr = NULL;
    vsi_nn_tensor_attr_t attr;
    vsi_nn_internal_tensor_t* output_tensor = NULL;
    vsi_nn_tensor_t** split_output_tensors = NULL;
    vsi_nn_tensor_t** rnncell_reshape_output_tensors_fw =NULL;
    vsi_nn_tensor_t** rnncell_reshape_output_tensors_bw =NULL;
    vsi_nn_tensor_t* last_step_h_state_fw = NULL;
    vsi_nn_tensor_t* last_step_h_state_bw = NULL;
    vsi_nn_tensor_t* tensor = NULL;
    vsi_nn_tensor_t* input_tensor = NULL;
    vsi_nn_tensor_t* aux_input_tensor = NULL;
    vsi_nn_tensor_t** aux_split_output_tensors = NULL;
    vsi_nn_tensor_t** reshape_output_tensors = NULL;
    vsi_nn_tensor_t** aux_reshape_output_tensors = NULL;
    vsi_bool has_aux_input = (inputs[BI_RNN_AUX_INPUT] != NULL);
    vsi_bool use_virtual_tensor = TRUE;
    vsi_size_t batch_size = 0;
    vsi_size_t time_step = 0;
    vsi_size_t i = 0;
    vsi_bool ret = FALSE;
    vsi_nn_tensor_t** merge_tensors = NULL;
    vsi_status status = VSI_FAILURE;

    memset(&attr, 0, sizeof(vsi_nn_tensor_attr_t));
    vsi_nn_internal_init_node_wksp( self );

    if( curr_param->time_major )
    {
        batch_size = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
        time_step = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
    }
    else
    {
        batch_size = inputs[BI_RNN_INPUT_INPUT]->attr.size[2];
        time_step = inputs[BI_RNN_INPUT_INPUT]->attr.size[1];
    }

    setup_op_shapes( self, inputs, outputs);

    /* default to input */
    input_tensor = inputs[BI_RNN_INPUT_INPUT];
    if( !curr_param->time_major )
    {
        /* transpose to time_major */
        output_tensor = vsi_nn_rnn_transpose_time_major(self,
            inputs[BI_RNN_INPUT_INPUT], NULL, use_virtual_tensor);
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        input_tensor = output_tensor->t;
    }

    /* default to aux input */
    if(has_aux_input)
    {
        aux_input_tensor = inputs[BI_RNN_AUX_INPUT];
        if( !curr_param->time_major )
        {
            /* transpose to time_major */
            output_tensor = vsi_nn_rnn_transpose_time_major(self,
                inputs[BI_RNN_AUX_INPUT], NULL, use_virtual_tensor);
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
            aux_input_tensor = output_tensor->t;
        }
    }

    /* split input tensor */
    split_output_tensors = (vsi_nn_tensor_t **)malloc(time_step * sizeof(vsi_nn_tensor_t **));
    CHECK_PTR_FAIL_GOTO( split_output_tensors, "Create buffer fail.", final );
    memset( split_output_tensors, 0x00, time_step * sizeof(vsi_nn_tensor_t **));
    reshape_output_tensors = (vsi_nn_tensor_t **)malloc(time_step * sizeof(vsi_nn_tensor_t **));
    CHECK_PTR_FAIL_GOTO( reshape_output_tensors, "Create buffer fail.", final );
    memset( reshape_output_tensors, 0x00, time_step * sizeof(vsi_nn_tensor_t **));

    status = vsi_nn_rnn_split_input_tensor(self, input_tensor,
        split_output_tensors, (uint32_t)time_step, use_virtual_tensor);
    CHECK_STATUS_FAIL_GOTO(status, final);

    status = vsi_nn_rnn_data_check_aligned(self, split_output_tensors, (uint32_t)time_step, use_virtual_tensor);
    CHECK_STATUS_FAIL_GOTO(status, final);

    /* split aux input tensor */
    if(has_aux_input)
    {
        aux_split_output_tensors = (vsi_nn_tensor_t **)malloc(time_step * sizeof(vsi_nn_tensor_t **));
        CHECK_PTR_FAIL_GOTO( aux_split_output_tensors, "Create buffer fail.", final );
        memset( aux_split_output_tensors, 0x00, time_step * sizeof(vsi_nn_tensor_t **));
        aux_reshape_output_tensors = (vsi_nn_tensor_t **)malloc(time_step * sizeof(vsi_nn_tensor_t **));
        CHECK_PTR_FAIL_GOTO( aux_reshape_output_tensors, "Create buffer fail.", final );
        memset( aux_reshape_output_tensors, 0x00, time_step * sizeof(vsi_nn_tensor_t **));

        status = vsi_nn_rnn_split_input_tensor(self, aux_input_tensor,
            aux_split_output_tensors, (uint32_t)time_step, use_virtual_tensor);
        CHECK_STATUS_FAIL_GOTO(status, final);

        status = vsi_nn_rnn_data_check_aligned(self, aux_split_output_tensors,
            (uint32_t)time_step, use_virtual_tensor);
        CHECK_STATUS_FAIL_GOTO(status, final);
    }

    /* prepare output tensor */
    rnncell_reshape_output_tensors_fw = (vsi_nn_tensor_t **)malloc(time_step *
        sizeof(vsi_nn_tensor_t **));
    CHECK_PTR_FAIL_GOTO( rnncell_reshape_output_tensors_fw, "Create buffer fail.", final );
    memset( rnncell_reshape_output_tensors_fw, 0x00, time_step * sizeof(vsi_nn_tensor_t **));
    rnncell_reshape_output_tensors_bw = (vsi_nn_tensor_t **)malloc(time_step *
        sizeof(vsi_nn_tensor_t **));
    CHECK_PTR_FAIL_GOTO( rnncell_reshape_output_tensors_bw, "Create buffer fail.", final );
    memset( rnncell_reshape_output_tensors_bw, 0x00, time_step * sizeof(vsi_nn_tensor_t **));

    for( i = 0; i < time_step; i++ )
    {
        /* reshape for split output */
        output_tensor = vsi_nn_rnn_reshape_split_output(self,
            split_output_tensors[i], (uint32_t)batch_size, use_virtual_tensor);
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        reshape_output_tensors[i] = output_tensor->t;

        if (has_aux_input)
        {
            /* reshape for aux split output */
            output_tensor = vsi_nn_rnn_reshape_split_output(self,
                aux_split_output_tensors[i], (uint32_t)batch_size, use_virtual_tensor);
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
            aux_reshape_output_tensors[i] = output_tensor->t;
        }
    }

    /* forward rnn op */
    last_step_h_state_fw = inputs[BI_RNN_FW_INPUT_H_STATE];
    for( i = 0; i < time_step; i++ )
    {
        vsi_nn_tensor_t* rnncell_out0 = NULL;
        vsi_nn_tensor_t* rnncell_out1 = NULL;

        /* rnncell output */
        vsi_nn_internal_init_tensor_attr(&attr,
            &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_out0 = output_tensor->t;

        /* rnncell output h_state */
        vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_FW_OUTPUT_H_STATE]->attr.dtype, use_virtual_tensor);
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_out1 = output_tensor->t;

        if (reshape_output_tensors[i]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_FW_INPUT_WEIGHT_I]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        if (last_step_h_state_fw &&
            last_step_h_state_fw->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_FW_INPUT_WEIGHT_H]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        if (has_aux_input &&
            aux_reshape_output_tensors[i]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_FW_AUX_INPUT_WEIGHT]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        curr = vsi_nn_internal_new_node( self, VSI_NN_OP_RNNCELL_OVXLIB, 0, 0 );
        CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
        curr->node->nn_param.rnncell_ovxlib.activation = curr_param->activation;
        memcpy( curr->node->nn_param.rnncell_ovxlib.internal_dtype,
            curr_param->internal_dtype,
            sizeof(vsi_nn_dtype_t) * RNNCELL_QUANTIZE_PARAM_COUNT);
        curr->inputs[RNNCELL_INPUT_INPUT] = reshape_output_tensors[i];
        curr->inputs[RNNCELL_INPUT_H_STATE] = last_step_h_state_fw;

        curr->inputs[RNNCELL_INPUT_WEIGHT_I] = inputs[BI_RNN_FW_INPUT_WEIGHT_I];
        curr->inputs[RNNCELL_INPUT_WEIGHT_H] = inputs[BI_RNN_FW_INPUT_WEIGHT_H];

        curr->inputs[RNNCELL_INPUT_BIAS_I] = inputs[BI_RNN_FW_INPUT_BIAS_I];
        curr->inputs[RNNCELL_INPUT_BIAS_H] = inputs[BI_RNN_FW_INPUT_BIAS_H];
        if (has_aux_input)
        {
            curr->inputs[RNNCELL_INPUT_AUX_INPUT] = aux_reshape_output_tensors[i];
            curr->inputs[RNNCELL_INPUT_AUX_WEIGHT] = inputs[BI_RNN_FW_AUX_INPUT_WEIGHT];
        }
        else
        {
            curr->inputs[RNNCELL_INPUT_AUX_INPUT] = NULL;
            curr->inputs[RNNCELL_INPUT_AUX_WEIGHT] = NULL;
        }

        curr->outputs[RNNCELL_OUTPUT_OUTPUT] = rnncell_out0;
        curr->outputs[RNNCELL_OUTPUT_H_STATE] = rnncell_out1;

        vsi_nn_internal_setup_node( self, curr );

        last_step_h_state_fw = rnncell_out1;

        /* reshape output to 3-dims */
        output_tensor = vsi_nn_rnn_reshape_cell_output(self,
            rnncell_out0, (uint32_t)batch_size, use_virtual_tensor);
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_reshape_output_tensors_fw[i] = output_tensor->t;
    }

    /* backward rnn op */
    last_step_h_state_bw = inputs[BI_RNN_BW_INPUT_H_STATE];
    for( i = 0; i < time_step; i++ )
    {
        vsi_nn_tensor_t* rnncell_out0 = NULL;
        vsi_nn_tensor_t* rnncell_out1 = NULL;

        /* rnncell output */
        if(curr_param->merge_outputs)
        {
            vsi_nn_internal_init_tensor_attr(&attr,
            &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
        }
        else
        {
            vsi_nn_internal_init_tensor_attr(&attr,
            &outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
        }
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_out0 = output_tensor->t;

        /* rnncell output h_state */
        vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_BW_OUTPUT_H_STATE]->attr.dtype, use_virtual_tensor);
        output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_out1 = output_tensor->t;

        if (reshape_output_tensors[time_step - 1 - i]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_BW_INPUT_WEIGHT_I]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_I].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        if (last_step_h_state_bw &&
            last_step_h_state_bw->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_BW_INPUT_WEIGHT_H]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_H].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        if (has_aux_input &&
            aux_reshape_output_tensors[time_step - 1 - i]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            inputs[BI_RNN_BW_AUX_INPUT_WEIGHT]->attr.dtype.vx_type == VSI_NN_TYPE_FLOAT32 &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].qnt_type == VSI_NN_QNT_TYPE_NONE &&
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].vx_type == VSI_NN_TYPE_NONE)
        {
            curr_param->internal_dtype[RNNCELL_QUANTIZE_PARAM_AUX].vx_type = VSI_NN_TYPE_FLOAT32;
        }

        curr = vsi_nn_internal_new_node( self, VSI_NN_OP_RNNCELL_OVXLIB, 0, 0 );
        CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
        curr->node->nn_param.rnncell_ovxlib.activation = curr_param->activation;
        memcpy( curr->node->nn_param.rnncell_ovxlib.internal_dtype,
                curr_param->internal_dtype,
                sizeof(vsi_nn_dtype_t) * RNNCELL_QUANTIZE_PARAM_COUNT);
        curr->inputs[RNNCELL_INPUT_INPUT] = reshape_output_tensors[time_step - 1 - i];
        curr->inputs[RNNCELL_INPUT_H_STATE] = last_step_h_state_bw;

        curr->inputs[RNNCELL_INPUT_WEIGHT_I] = inputs[BI_RNN_BW_INPUT_WEIGHT_I];
        curr->inputs[RNNCELL_INPUT_WEIGHT_H] = inputs[BI_RNN_BW_INPUT_WEIGHT_H];

        curr->inputs[RNNCELL_INPUT_BIAS_I] = inputs[BI_RNN_BW_INPUT_BIAS_I];
        curr->inputs[RNNCELL_INPUT_BIAS_H] = inputs[BI_RNN_BW_INPUT_BIAS_H];
        if(has_aux_input)
        {
            curr->inputs[RNNCELL_INPUT_AUX_INPUT] = aux_reshape_output_tensors[time_step - 1 - i];
            curr->inputs[RNNCELL_INPUT_AUX_WEIGHT] = inputs[BI_RNN_BW_AUX_INPUT_WEIGHT];
        }
        else
        {
            curr->inputs[RNNCELL_INPUT_AUX_INPUT] = NULL;
            curr->inputs[RNNCELL_INPUT_AUX_WEIGHT] = NULL;
        }

        curr->outputs[RNNCELL_OUTPUT_OUTPUT] = rnncell_out0;
        curr->outputs[RNNCELL_OUTPUT_H_STATE] = rnncell_out1;

        vsi_nn_internal_setup_node( self, curr );

        last_step_h_state_bw = rnncell_out1;

        /* reshape output to 3-dims */
        output_tensor = vsi_nn_rnn_reshape_cell_output(self,
            rnncell_out0, (uint32_t)batch_size, use_virtual_tensor);
        CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );
        rnncell_reshape_output_tensors_bw[time_step - 1 - i] = output_tensor->t;
    }

    if(curr_param->merge_outputs)
    {
        merge_tensors = (vsi_nn_tensor_t **)malloc(time_step * sizeof(vsi_nn_tensor_t **));
        CHECK_PTR_FAIL_GOTO( merge_tensors, "Create buffer fail.", final );
        memset( merge_tensors, 0x00, time_step * sizeof(vsi_nn_tensor_t **));

        tensor = outputs[BI_RNN_FW_OUTPUT_OUTPUT];
        if( !curr_param->time_major )
        {
            vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
            output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );

            tensor = output_tensor->t;
        }

        /* concat fw & bw output, the rnn's output is 3-dims */
        for( i = 0; i < time_step; i++ )
        {
            vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
            output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );

            curr = vsi_nn_internal_new_node( self, VSI_NN_OP_CONCAT, 2, 1 );
            CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
            curr->node->nn_param.concat.axis = 0;
            curr->inputs[0] = rnncell_reshape_output_tensors_fw[i];
            curr->inputs[1] = rnncell_reshape_output_tensors_bw[i];
            curr->outputs[0] = output_tensor->t;
            vsi_nn_internal_setup_node( self, curr );
            merge_tensors[i] = output_tensor->t;
        }


        /* concat rnncell output, the rnn's output is 3-dims */
        curr = vsi_nn_internal_new_node( self, VSI_NN_OP_CONCAT, (uint32_t)time_step, 1 );
        CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
        curr->node->nn_param.concat.axis = 2;
        for( i = 0; i < time_step; i++ )
        {
            curr->inputs[i] = merge_tensors[i];
        }
        curr->outputs[0] = tensor;
        vsi_nn_internal_setup_node( self, curr );

        if( !curr_param->time_major )
        {
            /* transpose time_major to batch_major*/
            vsi_nn_rnn_transpose_time_major(self,
                tensor, outputs[BI_RNN_FW_OUTPUT_OUTPUT], use_virtual_tensor);
        }
    }
    else
    {
        /* forward output*/
        tensor = outputs[BI_RNN_FW_OUTPUT_OUTPUT];
        if( !curr_param->time_major )
        {
            vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_FW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
            output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );

            tensor = output_tensor->t;
        }

        /* forward output state*/
        if (outputs[BI_RNN_FW_OUTPUT_H_STATE] != NULL)
        {
            curr = vsi_nn_internal_new_node( self, VSI_NN_OP_DATACONVERT, 0, 0 );
            CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
            curr->inputs[0] = last_step_h_state_fw;
            curr->outputs[0] = outputs[BI_RNN_FW_OUTPUT_H_STATE];
            vsi_nn_internal_setup_node(self, curr);
        }

        /* concat rnncell output, the rnn's output is 3-dims */
        curr = vsi_nn_internal_new_node( self, VSI_NN_OP_CONCAT, (uint32_t)time_step, 1 );
        CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
        curr->node->nn_param.concat.axis = 2;
        for( i = 0; i < time_step; i++ )
        {
            curr->inputs[i] = rnncell_reshape_output_tensors_fw[i];
        }
        curr->outputs[0] = tensor;
        vsi_nn_internal_setup_node( self, curr );

        if( !curr_param->time_major )
        {
            /* transpose time_major to batch_major*/
            vsi_nn_rnn_transpose_time_major(self,
                tensor, outputs[BI_RNN_FW_OUTPUT_OUTPUT], use_virtual_tensor);
        }

        /* backward output*/
        tensor = outputs[BI_RNN_BW_OUTPUT_OUTPUT];
        if( !curr_param->time_major )
        {
            vsi_nn_internal_init_tensor_attr(&attr,
                &outputs[BI_RNN_BW_OUTPUT_OUTPUT]->attr.dtype, use_virtual_tensor);
            output_tensor = vsi_nn_internal_new_tensor( self, &attr, 0.0f );
            CHECK_PTR_FAIL_GOTO( output_tensor, "Create internal tensor fail.", final );

            tensor = output_tensor->t;
        }

        /* backward output state*/
        if (outputs[BI_RNN_BW_OUTPUT_H_STATE] != NULL)
        {
            curr = vsi_nn_internal_new_node( self, VSI_NN_OP_DATACONVERT, 0, 0 );
            CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
            curr->inputs[0] = last_step_h_state_bw;
            curr->outputs[0] = outputs[BI_RNN_BW_OUTPUT_H_STATE];
            vsi_nn_internal_setup_node(self, curr);
        }

        /* concat rnncell output, the rnn's output is 3-dims */
        curr = vsi_nn_internal_new_node( self, VSI_NN_OP_CONCAT, (uint32_t)time_step, 1 );
        CHECK_PTR_FAIL_GOTO(curr, "Create internal node failed", final);
        curr->node->nn_param.concat.axis = 2;
        for( i = 0; i < time_step; i++ )
        {
            curr->inputs[i] = rnncell_reshape_output_tensors_bw[i];
        }
        curr->outputs[0] = tensor;
        vsi_nn_internal_setup_node( self, curr );

        if( !curr_param->time_major )
        {
            /* transpose time_major to batch_major*/
            vsi_nn_rnn_transpose_time_major(self,
                tensor, outputs[BI_RNN_BW_OUTPUT_OUTPUT], use_virtual_tensor);
        }
    }

    ret = TRUE;
final:
    vsi_nn_safe_free( split_output_tensors );
    vsi_nn_safe_free( aux_split_output_tensors )
    vsi_nn_safe_free( reshape_output_tensors );
    vsi_nn_safe_free( aux_reshape_output_tensors );
    vsi_nn_safe_free( rnncell_reshape_output_tensors_fw );
    vsi_nn_safe_free( rnncell_reshape_output_tensors_bw );
    vsi_nn_safe_free( merge_tensors );

    return ret;
} /* op_setup() */

static vsi_status op_deinit
    (
    vsi_nn_node_t * self
    )
{
    vsi_status status = VSI_SUCCESS;

    vsi_nn_safe_free(self->nn_param.bidirectional_sequence_rnn.internal_dtype);
    vsi_nn_internal_deinit_node_wksp( self );

    return status;
} /* op_deinit() */

static vsi_status op_init
    (
    vsi_nn_node_t * self
    )
{
    vsi_status status = VSI_SUCCESS;

    self->nn_param.bidirectional_sequence_rnn.internal_dtype = (vsi_nn_dtype_t *)
        malloc(sizeof(vsi_nn_dtype_t) * RNNCELL_QUANTIZE_PARAM_COUNT * 2);
    memset(self->nn_param.bidirectional_sequence_rnn.internal_dtype, 0,
        sizeof(vsi_nn_dtype_t) * RNNCELL_QUANTIZE_PARAM_COUNT * 2);

    return status;
} /* op_init() */

#ifdef __cplusplus
extern "C" {
#endif
/* Registrar */
DEF_OP_REG
    (
    /* op_name    */ BIDIRECTIONAL_SEQUENCE_RNN,
    /* init       */ op_init,
    /* compute    */ op_compute,
    /* deinit     */ op_deinit,
    /* check      */ op_check,
    /* setup      */ op_setup,
    /* optimize   */ op_optimize,
    /* input_num  */ BI_RNN_INPUT_CNT,
    /* output_num */ BI_RNN_OUTPUT_CNT
    );
#ifdef __cplusplus
}
#endif
