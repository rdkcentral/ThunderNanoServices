#ifndef MODULE_NAME
#define MODULE_NAME TransformationTest
#endif

#include <localtracer/localtracer.h>
#include <tracing/tracing.h>

#include <IRenderer.h>
#include <Transformation.h>

#include <sstream>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

void printMatrix(const Compositor::Matrix& m)
{
    std::stringstream s;

    s << std::endl << "matrix: " << std::endl;

    for (int i = 0; i < m.size(); i += 3 ){
        s << "| " << m[i] << " | " << m[i + 1] << " | " << m[i + 2] << " |" << std::endl;
    }

    TRACE_GLOBAL(Trace::Information, ("%s", s.str().c_str()));
}

int main(int argc, const char* argv[])
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

    Compositor::Matrix test1;
    Compositor::Transformation::Identity(test1);
    Compositor::Transformation::Translate(test1, 20, 20);
    printMatrix(test1);

    Compositor::Matrix test2;
    Compositor::Transformation::Identity(test2);
    float rads = Compositor::Transformation::ToRadials(-22);
    Compositor::Transformation::Rotate(test2, rads);
    printMatrix(test2);

    Compositor::Matrix test3;
    Compositor::Transformation::Multiply(test3, test2, test1);
    Compositor::Transformation::Scale(test3, 50, 50);
    Compositor::Transformation::Transform(test3, Compositor::Transformation::TRANSFORM_FLIPPED_180);
    printMatrix(test3);

    Compositor::Matrix test4;
    Compositor::Transformation::Projection(test4, 1280, 720, Compositor::Transformation::TRANSFORM_270);
    printMatrix(test4);

    Compositor::Matrix test5;
    Compositor::Box box1;

    box1.x = 20;
    box1.y = 20;
    box1.width = 100;
    box1.height = 50;

    Compositor::Transformation::ProjectBox(test5, box1, Compositor::Transformation::TRANSFORM_90, rads, test4);
    printMatrix(test5);

    Compositor::Matrix test6;
    Compositor::Transformation::Transpose(test6, test1);
    printMatrix(test6);

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    WPEFramework::Core::Singleton::Dispose();

    return 0;
}