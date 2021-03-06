#ifndef CAFFE_GREENTEA_LIBDNN_HPP_
#define CAFFE_GREENTEA_LIBDNN_HPP_
#include <string>
#include <vector>
#include "caffe/device.hpp"

#ifdef USE_GREENTEA
#include "caffe/greentea/greentea.hpp"
#include "viennacl/backend/opencl.hpp"
#include "viennacl/ocl/backend.hpp"
#include "viennacl/ocl/context.hpp"
#include "viennacl/ocl/device.hpp"
#include "viennacl/ocl/platform.hpp"
#endif  // USE_GREENTEA

#ifdef USE_CUDA
#include "cuda.h"
#include "nvrtc.h"
#endif  // USE_CUDA

namespace caffe {

typedef enum {
    // Stack the batch update into one GEMM block
    // (deterministic, 1 kernel call)
    LIBDNN_CONVOLUTION_WG_ALGO_DIRECT        = 0,
    // Use multiple GEMM blocks in parallel and update weights atomically
    // (non deterministic, 1 kernel call, not supported on all devices)
    LIBDNN_CONVOLUTION_WG_ALGO_ATOMIC        = 1,
    // Use multiple GEMM blocks and an intermediate buffer
    // reduce weight updates
    // (deterministic, >= 2 kernel calls)
    LIBDNN_CONVOLUTION_WG_ALGO_REDUCTION     = 2
} libdnnConvolutionWeightAlgo_t;

struct libdnn_config {
  libdnn_config() :
    in_shape(3, 1),
    out_shape(3, 1),
    kernel(1, 1),
    pad(1, 0),
    stride(1, 1),
    dilation(1, 0)
  {}
  device* dev_ptr = nullptr;
  std::vector<int_tp> in_shape;
  std::vector<int_tp> out_shape;
  std::vector<int_tp> kernel;
  std::vector<int_tp> pad;
  std::vector<int_tp> stride;
  std::vector<int_tp> dilation;
  int_tp group = 1;
  bool bias_term = false;
  bool fast_unsafe_math = false;
  bool weights_backward = true;
  bool bias_backward = true;
  libdnnConvolutionWeightAlgo_t wgalgo = LIBDNN_CONVOLUTION_WG_ALGO_DIRECT;
};


template<typename Dtype>
class libdnn_conv {
 public:
  explicit libdnn_conv(libdnn_config config);
  void forward(const Dtype* bottom_data, const Dtype* weight,
               const Dtype* bias,
               Dtype* top_data, int_tp batch_size);
  void backward(bool prop_down_data,
                const Dtype* top_data, const Dtype* top_diff,
                const Dtype* weight, Dtype* weight_diff,
                const Dtype* bias, Dtype* bias_diff,
                const Dtype* bottom_data, Dtype* bottom_diff,
                int_tp batch_size);

 protected:
  void generate_kernels();
  void compile_kernel();
  std::string generate_header();
  std::string generate_common_defs();
  std::string generate_fw_defs();
  std::string generate_bw_defs();
  std::string generate_wg_defs();
  std::string generate_fw_kernels(std::string name);
  std::string generate_bw_kernels(std::string name);
  std::string generate_wg_kernels(std::string name);
#ifdef USE_GREENTEA
  viennacl::ocl::program compile_kernels_opencl(viennacl::ocl::context *ctx);
#endif  // USE_GREETEA
#ifdef USE_CUDA
  nvrtcProgram compile_kernels_cuda();
#endif  // USE_CUDA
  template<class T>
  void add_def(std::stringstream& ss, const char* name, T value);  // NOLINT
  template<class T>
  void add_def(std::stringstream& ss, const std::string name, T value);  // NOLINT

 private:
  device* dev_ptr_;

#ifdef USE_GREENTEA
  viennacl::ocl::program ocl_program_;
#endif  // USE_GREENTEA

#ifdef USE_CUDA
  nvrtcProgram cuda_program_;
  CUmodule cuda_module_;
#endif  // USE_CUDA

  std::string kernel_;

  // Forward GEMM sizes
  int_tp M_FW_;
  int_tp MG_FW_;
  int_tp N_FW_;
  int_tp K_FW_;
  int_tp KG_FW_;

  // Backward GEMM sizes
  int_tp M_BW_;
  int_tp MG_BW_;
  int_tp N_BW_;
  int_tp K_BW_;
  int_tp KG_BW_;

  // Weight GEMM sizes
  int_tp M_WG_;
  int_tp MG_WG_;
  int_tp N_WG_;
  int_tp NG_WG_;
  int_tp K_WG_;

  // Convolution parameters
  int_tp num_axes_;
  int_tp fmaps_in_;
  int_tp fmaps_out_;
  int_tp group_;

  std::vector<int_tp> pad_;
  std::vector<int_tp> stride_;
  std::vector<int_tp> dilation_;
  std::vector<int_tp> kernel_shape_;
  std::vector<int_tp> im_in_shape_;
  std::vector<int_tp> im_out_shape_;

  // Compile and method flags
  bool weights_backward_;
  bool bias_backward_;
  bool fast_unsafe_math_;
  bool bias_term_;
  bool skip_range_check_;
  Dtype bias_multiplier_;
  libdnnConvolutionWeightAlgo_t wgalgo_;
};

}  // namespace caffe

#endif /* CAFFE_GREENTEA_LIBDNN_HPP_ */
