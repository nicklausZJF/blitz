#include <iostream>

#include <cuda.h>
#include <cuda_runtime_api.h>

#include "backends/backends.h"

using namespace blitz;

// N C H W
Shape input_shape(4);
// K C R S
Shape filter_shape(4);
// N K P Q
Shape output_shape(4);
// cpu workspace
Shape workspace_shape_cpu(1);
// gpu workspace
Shape workspace_shape_gpu(1);

void set_input_shape_nchw(size_t N, size_t C, size_t H, size_t W) {
  input_shape[0] = N;
  input_shape[1] = C;
  input_shape[2] = H;
  input_shape[3] = W;
}

void set_filter_shape_kcrs(size_t K, size_t C, size_t R, size_t S) {
  filter_shape[0] = K;
  filter_shape[1] = C;
  filter_shape[2] = R;
  filter_shape[3] = S;
}

void set_output_shape_nkpq(size_t N, size_t K, size_t P, size_t Q) {
  output_shape[0] = N;
  output_shape[1] = K;
  output_shape[2] = P;
  output_shape[3] = Q;
}

void compare_cpu_gpu(size_t size, float* output_cpu, float* output_gpu) {
  for (size_t i = 0; i < size; ++i) {
    if (output_cpu[i] > output_gpu[i] + 1e-3 ||
      output_cpu[i] < output_gpu[i] - 1e-3) {
      std::cout << "Index: " << i << ", CPU: " << output_cpu[i] <<
        ", GPU: " << output_gpu[i] << std::endl;
    }
  }
}

// conmpare gpu result to cpu result
void convolution_forward(
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w) {
  // set up cpu
  CPUTensor<float> input_cpu(input_shape);
  CPUTensor<float> filter_cpu(filter_shape);
  CPUTensor<float> output_cpu(output_shape);
  CPUTensor<float> workspace_cpu(output_shape);
  // set up gpu
  GPUTensor<float> input_gpu(input_shape);
  GPUTensor<float> filter_gpu(filter_shape);
  GPUTensor<float> output_gpu(output_shape);
  GPUTensor<float> workspace_gpu(filter_shape);
  // set up copy
  CPUTensor<float> output_copy(output_shape);
  // cpu convolution 
  Backend<CPUTensor, float>::Convolution2DForwardFunc(
    &input_cpu, &filter_cpu,
    pad_h, pad_w, 
    str_h, str_w,
    &workspace_cpu,
    &output_cpu);
  // gpu convolution
  Backend<GPUTensor, float>::Convolution2DForwardFunc(
    &input_gpu, &filter_gpu,
    pad_h, pad_w, 
    str_h, str_w,
    &workspace_gpu,
    &output_gpu);
  // copy from gpu to cpu
  cudaMemcpy(output_copy.data(), output_gpu.data(),
    output_gpu.size() * sizeof(float), cudaMemcpyDeviceToHost);
  compare_cpu_gpu(output_cpu.size(), output_cpu.data(), output_copy.data());
}

void convolution_backward(
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w) {
}

void convolution_update(
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w) {
}

int main(int argc, char** argv) {
  const size_t NUM_ARGS = 14;
  // phase N C H W R S K P Q pad_h pad_w str_h str_w
  if (argc != NUM_ARGS + 1) {
    std::cerr << "Not enough args!" << std::endl;
    exit(1);
  }
  // get args
  const std::string phase = std::string(argv[1]); 
  const size_t N = atoi(argv[2]);
  const size_t C = atoi(argv[3]);
  const size_t H = atoi(argv[4]);
  const size_t W = atoi(argv[5]);
  const size_t R = atoi(argv[6]);
  const size_t S = atoi(argv[7]);
  const size_t K = atoi(argv[8]);
  const size_t P = atoi(argv[9]);
  const size_t Q = atoi(argv[10]);
  const size_t pad_h = atoi(argv[11]);
  const size_t pad_w = atoi(argv[12]);
  const size_t str_h = atoi(argv[13]);
  const size_t str_w = atoi(argv[14]);
  // set shapes
  set_input_shape_nchw(N, C, H, W);
  set_filter_shape_kcrs(K, C, R, S);
  set_output_shape_nkpq(N, K, P, Q);
  // set workspace shape
  workspace_shape_gpu[0] =
    input_shape.size() + output_shape.size() + filter_shape.size();
  workspace_shape_cpu[0] = C * H * W * P * Q;
  // run convolution
  if (phase == "forward") {
    convolution_forward(pad_h, pad_w, str_h, str_w);
  } else if (phase == "backward") {
    convolution_backward(pad_h, pad_w, str_h, str_w);
  } else if (phase == "update") {
    convolution_update(pad_h, pad_w, str_h, str_w);
  }
  return 0;
}
