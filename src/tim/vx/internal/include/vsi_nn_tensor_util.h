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
/** @file */
#ifndef _VSI_NN_TENSOR_UTIL_H
#define _VSI_NN_TENSOR_UTIL_H

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include "vsi_nn_platform.h"
#include "vsi_nn_graph.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_types.h"
#include "utils/vsi_nn_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------
                Types
-------------------------------------------*/

/** Openvx tensor attribute IDs */
typedef enum
{
    VSI_NN_TENSOR_ATTR_DIM_NUM = 0x1,
    VSI_NN_TENSOR_ATTR_DTYPE = 0x2,
    VSI_NN_TENSOR_ATTR_SIZE = 0x4,
    VSI_NN_TENSOR_ATTR_FIXED_POINT_POS = 0x8,
    VSI_NN_TENSOR_ATTR_CONST = 0x10,
    VSI_NN_TENSOR_ATTR_HIGH_PRECISION = 0x20,
    VSI_NN_TENSOR_ATTR_ALL =  0xFF
} vsi_nn_vxtensor_attr_t;

/*-------------------------------------------
        Macros and Variables
-------------------------------------------*/

/** Check attribute bit,
 * @see vsi_nn_vxtensor_attr_t
 */
#define vsi_nn_hasattr( mask, attr )    (( mask & attr ) != 0)

/*-------------------------------------------
                  Functions
-------------------------------------------*/

/**
 * Create a new tensor
 * Create a new tensor with given attributes.
 *
 * @param[in] graph Graph handle
 * @param[in] attr Tensor attributes
 *
 * @return Tensor handle on success, or NULL otherwise.
 */
OVXLIB_API vsi_nn_tensor_t * vsi_nn_CreateTensor
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_attr_t * attr
    );

/**
 * Reinit openvx tensor handle
 * Free an exist openvx tensor handle and create a new for current tensor.
 *
 * @param[in] graph Graph handle
 * @param[in] tensor Tensor handle to reinit
 *
 * @return TRUE if on success, or FALSE otherwise.
 */
vsi_bool vsi_nn_TensorReinit
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor
    );

/**
 * Release tensor
 * Relase current tensor and set the handle to NULL.
 *
 * @param[in] tensor Tensor to release
 */
OVXLIB_API void vsi_nn_ReleaseTensor
    (
    vsi_nn_tensor_t ** tensor
    );

/**
 * Set tensor's vx attribute
 * The value should be type of vsi_nn_vxtensor_attr_t.
 *
 * @param[in] tensor Tensor handle
 * @param[in] attrs New attributes to update
 * @see vsi_nn_vxtensor_attr_t
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_SetTensorAttr
    (
    vsi_nn_tensor_t * tensor,
    const vsi_nn_vxtensor_attr_t attrs
    );

/**
 * Query tensor attribute
 * Query vxtensor attribute and update current tensor attributes.
 *
 * @param[in] tensor Tensor handle to query and update
 * @param[in] attrs VxAttributes to query and update
 * @see vsi_nn_vxtensor_attr_t
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_QueryTensorAttr
    (
    vsi_nn_tensor_t * tensor,
    const vsi_nn_vxtensor_attr_t attrs
    );

/**
 * Convert tensor to data
 * Read tensor memory to a user space buffer and return it.
 * @note User should free the malloc buffer with vsi_nn_Free.
 * @see vsi_nn_Free
 *
 * @param[in] graph Graph handle
 * @param[in] tensor Tensor handle
 *
 * @return Data buffer address.
 */
OVXLIB_API uint8_t * vsi_nn_ConvertTensorToData
    (
    const vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor
    );

/**
 * Convert tensor to data
 * Read tensor memory to a user space buffer
 * and return it by float32 format.
 * @note User should free the malloc buffer with vsi_nn_Free.
 * @see vsi_nn_Free
 *
 * @param[in] graph Graph handle
 * @param[in] tensor Tensor handle
 *
 * @return Data buffer address.
 */
OVXLIB_API float * vsi_nn_ConvertTensorToFloat32Data
    (
    vsi_nn_graph_t *graph,
    vsi_nn_tensor_t *tensor
    );

/*
 * @deprecated
 * @see vsi_nn_ConvertRawTensorToData2
 */
OVXLIB_API uint8_t * vsi_nn_ConvertRawTensorToData
    (
    vx_context context,
    vx_tensor tensor,
    vsi_size_t * dim,
    vx_enum  * data_format,
    vsi_size_t * size,
    vsi_size_t * stride_size,
    vx_tensor_addressing * addr,
    vx_enum accessor
    );

/**
 * Convert vxTensor to data
 * Read vxTensor memory to user space buffer
 * @note User should free the malloc buffer with vsi_nn_Free.
 * @see vsi_nn_Free
 * @todo Remove context, it can be returned by vx APIs.
 *
 * @param[in] context vxContext
 * @param[in] tensor VxTensor
 * @param[in] Ovxlib tensor attribute
 * @param[out] addr vxTensor addressing
 * @param[in] accessor Access mode
 *
 * @return Data buffer address.
 */
OVXLIB_API uint8_t * vsi_nn_ConvertRawTensorToData2
    (
    vx_context context,
    vx_tensor tensor,
    vsi_nn_tensor_attr_t * attr,
    vsi_size_t * stride_size,
    vx_tensor_addressing * addr,
    vx_enum accessor
    );

/**
 * Save tensor to text
 * Save tensor to a text file with given path.
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor Tensor handle.
 * @param[in] filename Filename to save.
 * @param[in] seperator Characters used to seperate the data.
 */
OVXLIB_API void vsi_nn_SaveTensorToText
    (
    vsi_nn_graph_t   * graph,
    vsi_nn_tensor_t  * tensor,
    const char       * filename,
    char             * seperator
    );

/**
 * Save tensor to text by float32 format
 * Save tensor to a text file with given path, all data will
 * be converted to float32.
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor Tensor handle.
 * @param[in] filename Filename to save.
 * @param[in] seperator Characters used to seperate the data.
 * @see vsi_nn_SaveTensorToText
 */
OVXLIB_API void vsi_nn_SaveTensorToTextByFp32
    (
    vsi_nn_graph_t   * graph,
    vsi_nn_tensor_t  * tensor,
    const char       * filename,
    char             * seperator
    );

/**
 * Save data to text
 * Save data to a text file with given path
 *
 * @param[in] filename Filename to save.
 * @param[in] data Data buffer address.
 * @param[in] data_szie Size of data buffer.
 * @param[in] data_format Data type.
 * @param[in] seperator Characters used to seperate the data.
 */
OVXLIB_API void vsi_nn_SaveDataToText
    (
    const char  * filename,
    uint8_t    * data,
    vsi_size_t     data_size,
    vsi_nn_type_e data_format,
    char        * seperator
    );

/**
 * Save tensor to binary file
 * Save tensor to a binary file with given path.
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor Tensor handle.
 * @param[in] filename Filename to save.
 */
OVXLIB_API void vsi_nn_SaveTensorToBinary
    (
    vsi_nn_graph_t   * graph,
    vsi_nn_tensor_t  * tensor,
    const char       * filename
    );

/**
 * Create tensor from data buffer
 * Create a new tensor and copy data to the tensor memory.
 *
 * @param[in] graph Graph handle.
 * @param[in] data Data buffer address.
 * @param[in] attr Tensor attributes.
 *
 * @return Tensor handle on success, or NULL otherwise.
 */
OVXLIB_API vsi_nn_tensor_t * vsi_nn_CreateTensorFromData
    (
    vsi_nn_graph_t       * graph,
    uint8_t             * data,
    vsi_nn_tensor_attr_t * attr
    );

/**
 * Copy data to tensor
 * Copy data from buffer to tensor memory.
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor Tensor handle.
 * @param[in] data Data buffer address.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_CopyDataToTensor
    (
    const vsi_nn_graph_t * graph,
    vsi_nn_tensor_t      * tensor,
    void                 * data
    );

/**
 * Swap a tensor's Handle
 * Swap handle to old_ptr to read/write, swap new handle to new_ptr to update handle.
 *
 * APP SHOULD maintain handle that created by itself to manage memory correctly,
 * never free or wirte data for handel allocated by OVXLIB.
 *
 * OVXLIB would not maintain original handle anymore if new_ptr == NULL.
 *
 * Before free data in handle allocated by APP, vsi_nn_SwapHandle(tensor, NULL, &prev_ptr)
 * should be called to get contol of handle.
 *
 * @param[in] tensor Tensor.
 * @param[in] new_ptr New handle of tensor.
 * @param[in] is_new_ptr_malloc_by_ovxlib If new_ptr is allocated by ovxlib while new_ptr is not NULL.
 * @param[out] old_ptr Old handle of tensor.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_SwapHandle
(
    vsi_nn_tensor_t* tensor,
    void* new_ptr,
    vsi_bool is_new_ptr_malloc_by_ovxlib,
    void** old_ptr
);

/**
 * Flush Handle
 * Call this function to flush new data to the handle in hand.
 * vsi_nn_FlushHandle() should be called at last to compleate the data writting operation.
 *
 * @param[in] tensor Tensor.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_FlushHandle
    (
    const vsi_nn_tensor_t * tensor
    );

/**
 * Invalidate Handle
 * invalidate handle before copy data from tensor handle.
 * Before read data in handle, vsi_nn_InvalidateHandle() should be called to do invalidate cache in APP.
 *
 * @param[in] tensor Tensor.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_InvalidateHandle
(
    const vsi_nn_tensor_t* tensor
);

/**
 * Get Tensor Handle
 * Get the handle of the tensor
 *
 * @param[in] tensor Tensor.
 * @param[out] ptr The handle of the tensor.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_GetTensorHandle
    (
    vsi_nn_tensor_t      * tensor,
    void** ptr
    );

/**
 * Get Tensor is_scalar
 * Get the is_scalar of the tensor
 *
 * @param[in] tensor Tensor.
 *
 * @return is_scalar flag of the tensor.
 */
OVXLIB_API int8_t vsi_nn_GetTensorIsScalar
(
    vsi_nn_tensor_t* tensor
);

/**
 * Set Tensor is_scalar
 * Set the is_scalar for the tensor
 *
 * @param[in] tensor Tensor.
 * @param[in] new is_scalar value of the tensor.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
OVXLIB_API vsi_status vsi_nn_SetTensorIsScalar
(
    vsi_nn_tensor_t* tensor,
    int8_t is_scalar
);

OVXLIB_API vsi_status vsi_nn_CopyRawDataToTensor
    (
    vsi_nn_graph_t*         graph,
    uint8_t*                src_data,
    const vsi_nn_dtype_t*   src_dtype,
    vsi_nn_tensor_t*        tensor
    );

OVXLIB_API vsi_size_t vsi_nn_CopyTensorToBuffer
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor,
    void            * buffer
    );

/**
 * Print node inputs and outputs
 * Print a brief info of a node inputs and outputs.
 * @todo move this to vsi_nn_node.h
 *
 * @param[in] graph Graph handle.
 * @param[in] node Node handle.
 */
OVXLIB_API void vsi_nn_PrintNodeIO
    (
    vsi_nn_graph_t *graph,
    vsi_nn_node_t *node
    );

/**
 * Print tensor
 * Print a brief info of a tensor.
 *
 * @param[in] tensor Tensor handle.
 * @param[in] id Tensor id.
 */
OVXLIB_API void vsi_nn_PrintTensor
    (
    vsi_nn_tensor_t * tensor,
    vsi_nn_tensor_id_t id
    );

OVXLIB_API void vsi_nn_TransposeTensor
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor,
    vsi_size_t       * perm,
    vsi_size_t         dim_num,
    vsi_size_t       * as_shape
    );

vx_tensor vsi_nn_safe_reshape_tensor
    (
    vx_tensor         tensor,
    void            * num_of_dims,
    vsi_size_t        sizes,
    vsi_size_t        size_of_shape_element
    );

OVXLIB_API void vsi_nn_PermuteTensor
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor,
    vsi_size_t       * perm,
    vsi_size_t         dim_num
    );

OVXLIB_API vsi_bool vsi_nn_CalcReshapeTensor
    (
    vsi_nn_tensor_t * input,
    vsi_nn_tensor_t * output,
    vsi_size_t       * shape,
    vsi_size_t         dim_num
    );

OVXLIB_API vsi_bool vsi_nn_ReshapeTensor
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * input,
    vsi_nn_tensor_t * output,
    const vsi_size_t  * shape,
    vsi_size_t         dim_num
    );

/**
 * Get element number of a tensor
 *
 * @param[in] tensor Tensor handle.
 * @return Element number of the tensor.
 */
OVXLIB_API vsi_size_t vsi_nn_GetElementNum
    (
    const vsi_nn_tensor_t * tensor
    );

/**
 * Get tensor size
 * The size is the bytes of the tensor memory reserved.
 *
 * @param[in] shape Shape handle.
 * @param[in] dim_num Dimension number.
 * @param[in] dtype Datatype.
 * @see vsi_nn_type_e
 *
 * @return Size of the tensor.
 */
OVXLIB_API vsi_size_t vsi_nn_GetTensorSize
    (
    const vsi_size_t * shape,
    vsi_size_t dim_num,
    vsi_nn_type_e dtype
    );

/**
 * Create a tensor by a scalar
 *
 * @todo Changed to vsi_nn_ScalarToTensor
 * @param[in] self Node handle ????
 * @param[in] data Scalar address.
 * @param[in] type Scalar data type.
 * @see vsi_nn_type_e
 *
 */
OVXLIB_API vsi_nn_tensor_t * vsi_nn_VariableToTensor
    (
    vsi_nn_node_t * self,
    uint8_t * data,
    vsi_nn_type_e type
    );

/**
 * Malloc a buffer
 *
 * @param[in] size Size to malloc.
 *
 * @return Buffer address.
 */
OVXLIB_API void *vsi_nn_Malloc
    (
    size_t size
    );

/**
 * Free a buffer
 * This API is used to free ovxlib malloc buffers.
 *
 * @param[in] data Data buffer address.
 */
OVXLIB_API void vsi_nn_Free
    (
    void * data
    );

/**
 * Create view vxTensor from an exist tensor
 * The new tensor is created from a tensor view of current tensor.
 *
 * @param[in] graph Graph handle.
 * @param[in] start View start region.
 * @param[in] end View end region.
 * @param[in] tensor Tensor handle to create the view.
 *
 * @return vxTensor from the view.
 */
OVXLIB_API vx_tensor vsi_nn_CreateViewTensor
    (
    vsi_nn_graph_t *graph,
    vsi_size_t *start,
    vsi_size_t *end,
    vsi_nn_tensor_t *tensor
    );

OVXLIB_API void vsi_nn_ReleaseTensorRelevance
    (
    vsi_nn_graph_t *graph,
    vsi_nn_tensor_rel_t *tensor_ref
    );

OVXLIB_API vsi_nn_tensor_rel_t *vsi_nn_CreateTensorRelevance
    (
    vsi_nn_graph_t *graph
    );

OVXLIB_API vsi_nn_tensor_t * vsi_nn_CreateTensorFromHandle
    (
    vsi_nn_graph_t       * graph,
    uint8_t              * data,
    vsi_nn_tensor_attr_t * attr
    );

OVXLIB_API vsi_status vsi_nn_SwapTensorHandle
    (
    vsi_nn_tensor_t * tensor0,
    vsi_nn_tensor_t * tensor1
    );

OVXLIB_API vsi_status vsi_nn_SwapTensorHandleWithCache
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * tensor0,
    vsi_nn_tensor_t * tensor1
    );

OVXLIB_API vsi_size_t vsi_nn_vxGetTensorElementNum
    (
    vsi_nn_tensor_attr_t *attr
    );

OVXLIB_API vsi_status vsi_nn_vxGetTensorAttr
    (
    vx_tensor tensor,
    vsi_nn_tensor_attr_t *attr
    );

OVXLIB_API uint8_t *vsi_nn_vxCopyTensorToData
    (
    vx_context context,
    vx_tensor tensor,
    vsi_nn_tensor_attr_t *attr
    );

OVXLIB_API vsi_status vsi_nn_vxCopyDataToTensor
    (
    vx_context context,
    vx_tensor tensor,
    vsi_nn_tensor_attr_t *attr,
    uint8_t *data
    );

/**
* Get offset by tensor coods
* Get offset by tensor coods.
*
* @param[in] tensor's attr
* @param[in] coords
*
* @return the offset from the beginning of the tensor(offset unit: element)
*/
OVXLIB_API vsi_size_t vsi_nn_GetOffsetByCoords
    (
    vsi_nn_tensor_attr_t *attr,
    uint32_t *coords
    );

/**
 * Create a tensor with attr and default value
 * the tensor content will be initialized with default value
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor attr.
 * @param[in] default value to be assigned to tensor content.
 *
 * @return new tensor on success, or NULL otherwise.
 */
OVXLIB_API vsi_nn_tensor_t * vsi_nn_CreateTensorWithDefault
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_attr_t * attr,
    float                  defualt_value
    );

/**
 * Fill tensor with specified value
 *
 * @param[in] graph Graph handle.
 * @param[in] target tensor.
 * @param[in] value to be assigned to tensor content.
 *
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
vsi_status vsi_nn_FillTensorWithValue
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_t      * tensor,
    float                  value
    );

void vsi_nn_print_node_io
    (
    vsi_nn_graph_t *graph,
    vsi_nn_node_t *node,
    int32_t type
    );

vsi_nn_tensor_t *vsi_nn_reshape_tensor
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * input,
    vsi_size_t        * shape,
    vsi_size_t          dim_num
    );

/**
 * OVXLIB internal tensor util api
 * A wrapper api for OpenVX vxCopyTensorPatch
 * Allows the application to copy a view patch from/into an tensor object .
 *
 * @param[in] tensor OpenVX Tensor handle.
 * @param[in] attr OVXLIB Tensor attr.
 * @param[in] user_ptr The address of the memory location where to store the requested data.
 * @param[in] start View start region.
 * @param[in] end View end region.
 * @param[in] stride Array of user memory strides in each dimension.
 * @param[in] usage This declares the effect of the copy with regard to the tensor object
 *            support VX_READ_ONLY or VX_WRITE_ONLY
 * @param[in] user_memory_type A, refer vx_memory_type_e
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
vsi_status vsi_nn_copy_tensor_veiw_patch
    (
    vx_tensor tensor,
    vsi_nn_tensor_attr_t *attr,
    void *user_ptr,
    vsi_size_t *start,
    vsi_size_t *end,
    vsi_size_t *stride,
    vsi_enum usage,
    vsi_enum user_memory_type
    );

/**
 * OVXLIB internal tensor util api
 * A wrapper api for OpenVX vxCopyTensorPatch
 * Allows the application to copy partial/whole tensor patch from/into an tensor object.
 *
 * @param[in] tensor OpenVX Tensor handle.
 * @param[in] attr OVXLIB Tensor attr.
 * @param[in] user_ptr The address of the memory location where to store the requested data.
 * @param[in] usage This declares the effect of the copy with regard to the tensor object
 *            support VX_READ_ONLY or VX_WRITE_ONLY
 * @param[in] start The start cooridinates for each dim. NULL means copy from the idx 0 of each dim.
 * @param[in] end The end cooridinates for each dim. NULL means copy to the end of each dim.
 * @return VSI_SUCCESS on success, or error core otherwise.
 */
vsi_status vsi_nn_copy_tensor_patch
    (
    vx_tensor tensor,
    vsi_nn_tensor_attr_t *attr,
    void * user_ptr,
    vsi_enum usage,
    vsi_size_t* start,
    vsi_size_t* end
    );

/**
 * OVXLIB internal tensor util api
 * Rotate 180 degrees in width*height*channel dims for weights data
 *
 * @param[in] graph Graph handle.
 * @param[in] weights tensor.
 */
void vsi_nn_reshuffle_weight_data
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * weights
    );

vsi_nn_tensor_t* vsi_nn_ConcatTensor_impl
    (
    vsi_nn_graph_t* graph,
    uint32_t axis,
    ...
    );
#define vsi_nn_ConcatTensor(_graph, _axis, ...) \
    vsi_nn_ConcatTensor_impl(_graph, _axis, __VA_ARGS__, END_OF_VARIADIC_ARGUMENTS)

/**
 * Add multiple constant tensor
 * All the input and output tensors must have the same shape.
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor attr.
 * @param[in] input constant tensors.
 *
 * @return new constant tensor on success, or NULL otherwise.
 */
vsi_nn_tensor_t* vsi_nn_ConstTensorAdd_impl
    (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_attr_t output_attr,
    ...
    );
#define vsi_nn_ConstTensorAdd(_graph, _output_attr, ...) \
    vsi_nn_ConstTensorAdd_impl(_graph, _output_attr, __VA_ARGS__, END_OF_VARIADIC_ARGUMENTS)

vsi_bool vsi_nn_ConvertTensor
    (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_t* input,
    vsi_nn_tensor_t* output
    );

vsi_nn_tensor_t * vsi_nn_dropout_tensor
    (
    vsi_nn_graph_t  * graph,
    vsi_nn_tensor_t * input,
    float             rate
    );

/**
 * Allows the application to get direct access to a patch of tensor object.
 * A wrapper api for OpenVX vxMapTensorPatch
 *
 * @param[in] graph Graph handle.
 * @param[in] tensor Tensor handle.
 * @param[out]  ptr The address of a pointer that the function sets to the
 * address where the requested data can be accessed. The returned (*ptr) address
 * is only valid between the call to the function and the corresponding call to
 * vsi_nn_UnmapTensorPatch.
 * @param [in] usage This declares the access mode for the tensor patch, using
 * the vsi_nn_accessor_type_e enumeration.
 * VSI_NN_READ_ONLY: after the function call, the content of the memory location
 * pointed by (*ptr) contains the tensor patch data. Writing into this memory location
 * is forbidden and its behavior is undefined.
 * VSI_NN_READ_AND_WRITE : after the function call, the content of the memory
 * location pointed by (*ptr) contains the tensor patch data; writing into this memory
 * is allowed only for the location of items and will result in a modification of the
 * affected items in the tensor object once the range is unmapped. Writing into
 * a gap between items (when (*stride) > item size in bytes) is forbidden and its
 * behavior is undefined.
 * VSI_NN_WRITE_ONLY: after the function call, the memory location pointed by (*ptr)
 * contains undefined data; writing each item of the range is required prior to
 * unmapping. Items not written by the application before unmap will become
 * undefined after unmap, even if they were well defined before map. Like for
 * VSI_NN_READ_AND_WRITE, writing into a gap between items is forbidden and its behavior
 * is undefined.
 * @return VSI_SUCCESS on success, or error core otherwise.
 */

OVXLIB_API vsi_status vsi_nn_MapTensorPatch
   (
    vsi_nn_graph_t* graph,
    vsi_nn_tensor_t* tensor,
    void** ptr,
    vsi_nn_accessor_type_e usage
   );

/**
 * Unmap and commit potential changes to a tensor object patch that was previously mapped.
 * Unmapping a tensor patch invalidates the memory location from which the patch could
 * be accessed by the application. Accessing this memory location after the unmap function
 * completes has an undefined behavior.
 * @param[in] graph Graph handle.
 * @param [in] tensor The reference to the tensor object to unmap.
 * return VSI_SUCCESS on success, or error core otherwise.
 */

OVXLIB_API vsi_status vsi_nn_UnmapTensorPatch
   (
   vsi_nn_graph_t* graph,
   vsi_nn_tensor_t* tensor
   );

/**
 * Create a new tensor from internal AXI-SRAM(Kernel driver maped)
 * It just creates the tensor object and does not actually allocate the memory
 * in AXI-SRAM until the verify graph stage. In the other words, the tensor
 * object is created beforehand,but the memory for storing its data is not
 * allocate until verify graph stage. AXI-SRAM is the internal memory resource
 * that memory allocation is done strategically to optimize performance and
 * resource usage in graph verification.
 * If there is no enough memory in AXI-SRAM, vsi_nn_VerifyGraph will return VSI_FAILURE
 * User can't access the tensor memory(read/write tensor data) before the graph has verified,
 * since the tensor memory is not allocated.
 * @param[in] graph Graph handle
 * @param[in] attr Tensor attirbutes to the new tensor.
 *
 * @return Tensor handle on success, or NULL otherwise.
 */
OVXLIB_API vsi_nn_tensor_t * vsi_nn_CreateTensorFromAXISRAM
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_attr_t * attr
    );

#ifdef __cplusplus
}
#endif

#endif
