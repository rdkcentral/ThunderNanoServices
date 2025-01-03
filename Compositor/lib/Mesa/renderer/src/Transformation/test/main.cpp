#ifndef MODULE_NAME
#define MODULE_NAME TransformationTest
#endif

#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <IRenderer.h>
#include <Transformation.h>


extern "C" {
#include "ref_matrix/ref_matrix.h"
}


#include <sstream>

using namespace Thunder;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

void printMatrix(const Compositor::Matrix& m)
{
    std::stringstream s;

    s << std::endl
      << "matrix: " << std::endl;

    for (uint32_t i = 0; i < m.size(); i += 3) {
        s << " | " << m[i] << " | " << m[i + 1] << " | " << m[i + 2] << " |" << std::endl;
    }

    TRACE_GLOBAL(Trace::Information, ("compositor %s", s.str().c_str()));
}

void printMatrix(const float m[9])
{
    std::stringstream s;

    s << std::endl
      << "matrix: " << std::endl;

    for (int i = 0; i < 9; i += 3) {
        s << "\t\t\t\t| " << m[i] << " | " << m[i + 1] << " | " << m[i + 2] << " |" << std::endl;
    }

    TRACE_GLOBAL(Trace::Information, ("wr %s", s.str().c_str()));
}

float round_to(float value, float precision = 0.00001)
{
    return std::round(value / precision) * precision;
}

bool compareResult(const Compositor::Matrix& ml, const float wr[9])
{
    return ( //
        (round_to(ml[0]) == round_to(wr[0])) //
        && (round_to(ml[1]) == round_to(wr[1])) //
        && (round_to(ml[2]) == round_to(wr[2])) //
        && (round_to(ml[3]) == round_to(wr[3])) //
        && (round_to(ml[4]) == round_to(wr[4])) //
        && (round_to(ml[5]) == round_to(wr[5])) //
        && (round_to(ml[6]) == round_to(wr[6])) //
        && (round_to(ml[7]) == round_to(wr[7])) //
        && (round_to(ml[8]) == round_to(wr[8])) //
    );
}

int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    std::vector<string> modules = {
        "Error",
        "Information"
    };

    for (auto module : modules) {
        tracer.EnableMessage("TransformationTest", module, true);
    }

    const char* executableName(Core::FileNameOnly(argv[0]));

    printf("%s - build: %s\n", executableName, __TIMESTAMP__);

    /* ############################################################## */

    Compositor::Matrix test1;
    Compositor::Transformation::Identity(test1);
    Compositor::Transformation::Translate(test1, 20, 20);
    printMatrix(test1);

    float wr1[9];
    ref_matrix_identity(wr1);
    ref_matrix_translate(wr1, 20, 20);
    printMatrix(wr1);

    ASSERT(compareResult(test1, wr1));

    /* ############################################################## */

    float rads = Compositor::Transformation::ToRadials(-22);

    Compositor::Matrix test2;
    Compositor::Transformation::Identity(test2);
    Compositor::Transformation::Rotate(test2, rads);
    printMatrix(test2);

    float wr2[9];
    ref_matrix_identity(wr2);
    ref_matrix_rotate(wr2, rads);
    printMatrix(wr2);

    ASSERT(compareResult(test2, wr2));

    /* ############################################################## */

    Compositor::Matrix test3;
    Compositor::Transformation::Multiply(test3, test2, test1);
    Compositor::Transformation::Scale(test3, 50, 50);
    Compositor::Transformation::Transform(test3, Compositor::Transformation::TRANSFORM_FLIPPED_180);
    // printMatrix(test3);

    float wr3[9];
    ref_matrix_multiply(wr3, wr2, wr1);
    ref_matrix_scale(wr3, 50, 50);
    ref_matrix_transform(wr3, REF_TRANSFORM_FLIPPED_180);
    // printMatrix(wr3);

    ASSERT(compareResult(test3, wr3));

    /* ############################################################## */

    Compositor::Matrix test4;
    Compositor::Transformation::Projection(test4, 1280, 720, Compositor::Transformation::TRANSFORM_270);
    printMatrix(test4);

    float wr4[9];
    ref_matrix_projection(wr4, 1280, 720, REF_TRANSFORM_270);
    printMatrix(wr4);

    ASSERT(compareResult(test4, wr4));

    /* ############################################################## */

    Compositor::Matrix test5;
    Exchange::IComposition::Rectangle box1 = { 20, 20, 100, 50 };
    Compositor::Transformation::ProjectBox(test5, box1, Compositor::Transformation::TRANSFORM_90, -0.383972, test4);
    printMatrix(test5);

    float wr5[9];
    ref_box box2 = { 20, 20, 100, 50 };
    ref_matrix_project_box(wr5, &box2, REF_TRANSFORM_90, -0.383972, wr4);
    printMatrix(wr5);

    ASSERT(compareResult(test5, wr5));

    /* ############################################################## */

    Compositor::Matrix test6;
    Compositor::Transformation::Transpose(test6, test1);
    printMatrix(test6);

    float wr6[9];
    ref_matrix_transpose(wr6, wr1);
    // printMatrix(wr6);

    ASSERT(compareResult(test6, wr6));

    /* ############################################################## */

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    Thunder::Core::Singleton::Dispose();

    return 0;
}