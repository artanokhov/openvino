// Copyright (C) 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <functional_test_utils/layer_test_utils.hpp>
#include <ngraph_functions/builders.hpp>
#include <vpu/ngraph/operations/dynamic_shape_resolver.hpp>
#include <vpu/myriad_plugin_config.hpp>

namespace {

using DataType = ngraph::element::Type_t;
using DataDims = ngraph::Shape;

using Parameters = std::tuple<
    DataType,
    DataDims,
    LayerTestsUtils::TargetDevice
>;

class NonZero_Transpose : public testing::WithParamInterface<Parameters>, public LayerTestsUtils::LayerTestsCommon {
protected:
    void SetUp() override {
        const auto& parameters = GetParam();
        const auto& dataType = std::get<0>(GetParam());
        const auto& dataDims = std::get<1>(GetParam());
        targetDevice = std::get<2>(GetParam());

        const auto data = std::make_shared<ngraph::opset3::Parameter>(dataType, dataDims);
        const auto nonZero = std::make_shared<ngraph::opset3::NonZero>(data);

        auto permutation = std::vector<std::int64_t>(dataDims.size());
        std::iota(permutation.begin(), permutation.end(), 0);
        std::shuffle(permutation.begin(), permutation.end(), std::mt19937());
        const auto transposition = std::make_shared<ngraph::opset3::Constant>(ngraph::element::i64, ngraph::Shape{dataDims.size()}, permutation);
        const auto transpose = std::make_shared<ngraph::opset3::Transpose>(nonZero, transposition);

        const auto result = std::make_shared<ngraph::opset3::Result>(transpose);
        function = std::make_shared<ngraph::Function>(ngraph::ResultVector{result}, ngraph::ParameterVector{data}, "NonZero-Transpose");
    }
};

TEST_P(NonZero_Transpose, CompareWithReference) {
    SKIP_IF_CURRENT_TEST_IS_DISABLED()

    configuration.emplace(VPU_MYRIAD_CONFIG_KEY(PLATFORM), VPU_MYRIAD_CONFIG_VALUE(2480));
    ConfigurePlugin();

    ASSERT_NO_THROW(LoadNetwork());
}

INSTANTIATE_TEST_CASE_P(DynamicTranspose, NonZero_Transpose,
    ::testing::Combine(
        ::testing::Values(ngraph::element::f16, ngraph::element::f32, ngraph::element::i32),
        ::testing::Values(ngraph::Shape{1, 800}),
        ::testing::Values(CommonTestUtils::DEVICE_MYRIAD)));

}  // namespace
