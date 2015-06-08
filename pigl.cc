#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <node.h>
#include <nan.h>
#include <v8.h>
//#include "arch_wrapper.h"

#include "bcm_host.h"

//#include <GL/glew.h>
#include "pigl.h"
#include "GLES/gl.h"
#include "GLES2/gl2.h"
#include "EGL/egl.h"
//#include "EGL/eglext.h"

using namespace node;
using namespace v8;
using namespace std;

typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
// GLuint tex[6];
} TEST_STATE_T;

// forward declarations
enum GLObjectType {
  GLOBJECT_TYPE_BUFFER,
  GLOBJECT_TYPE_FRAMEBUFFER,
  GLOBJECT_TYPE_PROGRAM,
  GLOBJECT_TYPE_RENDERBUFFER,
  GLOBJECT_TYPE_SHADER,
  GLOBJECT_TYPE_TEXTURE,
};

struct GLObj {
  GLObjectType type;
  GLuint obj;
  GLObj(GLObjectType type, GLuint obj) {
    this->type=type;
    this->obj=obj;
  }
};

vector<GLObj*> globjs;
static bool atExit=false;

void registerGLObj(GLObjectType type, GLuint obj);
void unregisterGLObj(GLuint obj);

NAN_METHOD(VertexAttrib1f);
NAN_METHOD(VertexAttrib2f);
NAN_METHOD(VertexAttrib3f);
NAN_METHOD(VertexAttrib4f);
NAN_METHOD(VertexAttrib1fv);
NAN_METHOD(VertexAttrib2fv);
NAN_METHOD(VertexAttrib3fv);
NAN_METHOD(VertexAttrib4fv);

NAN_METHOD(GetError);

NAN_METHOD(BlendColor);
NAN_METHOD(BlendEquationSeparate);
NAN_METHOD(BlendFuncSeparate);
NAN_METHOD(ClearStencil);
NAN_METHOD(ColorMask);
NAN_METHOD(CopyTexImage2D);
NAN_METHOD(CopyTexSubImage2D);
NAN_METHOD(CullFace);
NAN_METHOD(DepthMask);
NAN_METHOD(DepthRange);
NAN_METHOD(DisableVertexAttribArray);
NAN_METHOD(Hint);
NAN_METHOD(IsEnabled);
NAN_METHOD(LineWidth);
NAN_METHOD(PolygonOffset);

NAN_METHOD(Scissor);
NAN_METHOD(StencilFunc);
NAN_METHOD(StencilFuncSeparate);
NAN_METHOD(StencilMask);
NAN_METHOD(StencilMaskSeparate);
NAN_METHOD(StencilOp);
NAN_METHOD(StencilOpSeparate);
NAN_METHOD(BindRenderbuffer);
NAN_METHOD(CreateRenderbuffer);

NAN_METHOD(DeleteBuffer);
NAN_METHOD(DeleteFramebuffer);
NAN_METHOD(DeleteProgram);
NAN_METHOD(DeleteRenderbuffer);
NAN_METHOD(DeleteShader);
NAN_METHOD(DeleteTexture);
NAN_METHOD(DetachShader);
NAN_METHOD(FramebufferRenderbuffer);
NAN_METHOD(GetVertexAttribOffset);

NAN_METHOD(IsBuffer);
NAN_METHOD(IsFramebuffer);
NAN_METHOD(IsProgram);
NAN_METHOD(IsRenderbuffer);
NAN_METHOD(IsShader);
NAN_METHOD(IsTexture);

NAN_METHOD(RenderbufferStorage);
NAN_METHOD(GetShaderSource);
NAN_METHOD(ValidateProgram);

NAN_METHOD(ActiveTexture);

NAN_METHOD(TexSubImage2D);
NAN_METHOD(ReadPixels);
NAN_METHOD(GetTexParameter);
NAN_METHOD(GetActiveAttrib);
NAN_METHOD(GetActiveUniform);
NAN_METHOD(GetAttachedShaders);
NAN_METHOD(GetParameter);
NAN_METHOD(GetBufferParameter);
NAN_METHOD(GetFramebufferAttachmentParameter);
NAN_METHOD(GetProgramInfoLog);
NAN_METHOD(GetRenderbufferParameter);
NAN_METHOD(GetVertexAttrib);
NAN_METHOD(GetSupportedExtensions);
NAN_METHOD(GetExtension);
NAN_METHOD(CheckFramebufferStatus);
/*GLenum mode*/
NAN_METHOD(FrontFace);
NAN_METHOD(ClearColor);
NAN_METHOD(ClearDepth);
NAN_METHOD(BlendEquation);
NAN_METHOD(BlendFunc);
NAN_METHOD(Viewport);
NAN_METHOD(CreateBuffer);
NAN_METHOD(Clear);
NAN_METHOD(BindBuffer);
NAN_METHOD(BufferData);
NAN_METHOD(CreateProgram);
NAN_METHOD(CreateShader);
NAN_METHOD(ShaderSource);
NAN_METHOD(CompileShader);
NAN_METHOD(GetShaderParameter);
NAN_METHOD(GetShaderInfoLog);
NAN_METHOD(AttachShader);
NAN_METHOD(LinkProgram);
NAN_METHOD(GetProgramParameter);
NAN_METHOD(GetUniformLocation);
NAN_METHOD(GetAttribLocation);
NAN_METHOD(UseProgram);

NAN_METHOD(Uniform1f);
NAN_METHOD(Uniform2f);
NAN_METHOD(Uniform3f);
NAN_METHOD(Uniform4f);
NAN_METHOD(Uniform1i);
NAN_METHOD(Uniform2i);
NAN_METHOD(Uniform3i);
NAN_METHOD(Uniform4i);
NAN_METHOD(Uniform1fv);
NAN_METHOD(Uniform2fv);
NAN_METHOD(Uniform3fv);
NAN_METHOD(Uniform4fv);
NAN_METHOD(Uniform1iv);
NAN_METHOD(Uniform2iv);
NAN_METHOD(Uniform3iv);
NAN_METHOD(Uniform4iv);
NAN_METHOD(UniformMatrix2fv);
NAN_METHOD(UniformMatrix3fv);

NAN_METHOD(UniformMatrix4fv);
NAN_METHOD(EnableVertexAttribArray);
NAN_METHOD(DisableVertexAttribArray);
NAN_METHOD(VertexAttribPointer);
NAN_METHOD(DrawElements);

static TEST_STATE_T _state, *state=&_state;
static int32_t init_ogl(TEST_STATE_T *state);

static int SizeOfArrayElementForType(v8::ExternalArrayType type) {
  switch (type) {
  case v8::kExternalByteArray:
  case v8::kExternalUnsignedByteArray:
    return 1;
  case v8::kExternalShortArray:
  case v8::kExternalUnsignedShortArray:
    return 2;
  case v8::kExternalIntArray:
  case v8::kExternalUnsignedIntArray:
  case v8::kExternalFloatArray:
    return 4;
  default:
    return 0;
  }
}
static int32_t init_ogl(TEST_STATE_T *state){

   EGLint num_config;
   EGLBoolean result;
   EGLConfig config;
   int32_t success = 0;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);

   // create an EGL window surface, passing context width/height
   success = graphics_get_display_size(0 /* LCD */,
                        &state->screen_width, &state->screen_height);

   if (success < 0) {
       return EGL_FALSE;
   }

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );

   dispman_element = vc_dispmanx_element_add (
      dispman_update,
      dispman_display,
      0/*layer*/,
      &dst_rect,
      0/*src*/,
      &src_rect,
      DISPMANX_PROTECTION_NONE,
      0/*alpha*/,
      0/*clamp*/,
      DISPMANX_NO_ROTATE/*transform*/
   );

   nativewindow.element = dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );

   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);

   glClearColor(0.85f, 0.25f, 0.35f, 1.0f);

   // Enable back face culling.
   glEnable(GL_CULL_FACE);
   glMatrixMode(GL_MODELVIEW);

   glClear( GL_COLOR_BUFFER_BIT );
   eglSwapBuffers(state->display, state->surface);
   return result;
}

template<typename Type>
inline Type* getArrayData(Local<Value> arg, int* num = NULL) {
  Type *data=NULL;
  if(num) *num=0;

  if(!arg->IsNull()) {
    if(arg->IsArray()) {
      Local<Array> arr = Local<Array>::Cast(arg);
      if(num) *num=arr->Length();
      data = reinterpret_cast<Type*>(arr->GetIndexedPropertiesExternalArrayData());
    }
    else if(arg->IsObject()) {
      if(num) *num = arg->ToObject()->GetIndexedPropertiesExternalArrayDataLength();
      data = reinterpret_cast<Type*>(arg->ToObject()->GetIndexedPropertiesExternalArrayData());
    }
    else
    NanThrowError("Bad array argument");
  }

  return data;
}

void Method(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  bcm_host_init();

  memset( state, 0, sizeof( *state ) );
  // Start OGLES
  init_ogl(state);

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
}

NAN_METHOD(ActiveTexture) {
  NanScope();

  glActiveTexture(args[0]->Int32Value());
  NanReturnUndefined();
}

NAN_METHOD(Uniform1f) {
  NanScope();
  int location = args[0]->Int32Value();
  float x = (float) args[1]->NumberValue();

  glUniform1f(location, x);
  NanReturnUndefined();
}
NAN_METHOD(Uniform2f) {
  NanScope();

  int location = args[0]->Int32Value();
  float x = (float) args[1]->NumberValue();
  float y = (float) args[2]->NumberValue();

  glUniform2f(location, x, y);
  NanReturnUndefined();
}

NAN_METHOD(Uniform3f) {
  NanScope();

  int location = args[0]->Int32Value();
  float x = (float) args[1]->NumberValue();
  float y = (float) args[2]->NumberValue();
  float z = (float) args[3]->NumberValue();

  glUniform3f(location, x, y, z);
  NanReturnUndefined();
}

NAN_METHOD(Uniform4f) {
  NanScope();

  int location = args[0]->Int32Value();
  float x = (float) args[1]->NumberValue();
  float y = (float) args[2]->NumberValue();
  float z = (float) args[3]->NumberValue();
  float w = (float) args[4]->NumberValue();

  glUniform4f(location, x, y, z, w);
  NanReturnUndefined();
}

NAN_METHOD(Uniform1i) {
  NanScope();

  int location = args[0]->Int32Value();
  int x = args[1]->Int32Value();

  glUniform1i(location, x);
  NanReturnUndefined();
}

NAN_METHOD(Uniform2i) {
  NanScope();

  int location = args[0]->Int32Value();
  int x = args[1]->Int32Value();
  int y = args[2]->Int32Value();

  glUniform2i(location, x, y);
  NanReturnUndefined();
}

NAN_METHOD(Uniform3i) {
  NanScope();

  int location = args[0]->Int32Value();
  int x = args[1]->Int32Value();
  int y = args[2]->Int32Value();
  int z = args[3]->Int32Value();

  glUniform3i(location, x, y, z);
  NanReturnUndefined();
}

NAN_METHOD(Uniform4i) {
  NanScope();

  int location = args[0]->Int32Value();
  int x = args[1]->Int32Value();
  int y = args[2]->Int32Value();
  int z = args[3]->Int32Value();
  int w = args[4]->Int32Value();

  glUniform4i(location, x, y, z, w);
  NanReturnUndefined();
}

NAN_METHOD(Uniform1fv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLfloat *ptr=getArrayData<GLfloat>(args[1],&num);
  glUniform1fv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform2fv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLfloat *ptr=getArrayData<GLfloat>(args[1],&num);
  num /= 2;

  glUniform2fv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform3fv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLfloat *ptr=getArrayData<GLfloat>(args[1],&num);
  num /= 3;

  glUniform3fv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform4fv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLfloat *ptr=getArrayData<GLfloat>(args[1],&num);
  num /= 4;

  glUniform4fv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform1iv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLint *ptr=getArrayData<GLint>(args[1],&num);

  glUniform1iv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform2iv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLint *ptr=getArrayData<GLint>(args[1],&num);
  num /= 2;

  glUniform2iv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform3iv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLint *ptr=getArrayData<GLint>(args[1],&num);
  num /= 3;
  glUniform3iv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(Uniform4iv) {
  NanScope();

  int location = args[0]->Int32Value();
  int num=0;
  GLint *ptr=getArrayData<GLint>(args[1],&num);
  num /= 4;
  glUniform4iv(location, num, ptr);
  NanReturnUndefined();
}

NAN_METHOD(PixelStorei){
  NanScope();
  NanReturnUndefined();
}

/*WebGLProgram? program, GLuint index, DOMString name*/
/*NAN_METHOD(BindAttribLocation){
    NanScope();
    int program = args[0]->Int32Value();
    int index = args[1]->Int32Value();
    String::Utf8Value name(args[2]);

    glBindAttribLocation(program,  index,  *name);
    NanReturnUndefined();
}*/
NAN_METHOD(GetError){
    NanScope();
    NanReturnValue(NanNew<Number>(glGetError()));
}

NAN_METHOD(DrawArrays){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(UniformMatrix2fv) {
  NanScope();

  GLint location = args[0]->Int32Value();
  GLboolean transpose = args[1]->BooleanValue();

  GLsizei count=0;
  GLfloat* data=getArrayData<GLfloat>(args[2],&count);

  if (count < 4) {
    NanThrowError("Not enough data for UniformMatrix2fv");
  }

  glUniformMatrix2fv(location, count / 4, transpose, data);

  NanReturnUndefined();
}

NAN_METHOD(UniformMatrix3fv) {
  NanScope();

  GLint location = args[0]->Int32Value();
  GLboolean transpose = args[1]->BooleanValue();
  GLsizei count=0;
  GLfloat* data=getArrayData<GLfloat>(args[2],&count);

  if (count < 9) {
    NanThrowError("Not enough data for UniformMatrix3fv");
  }

  glUniformMatrix3fv(location, count / 9, transpose, data);

  NanReturnUndefined();
}

/*GLint location, GLsizei count, GLboolean transpose, const GLfloat *value*/
NAN_METHOD(UniformMatrix4fv){
  NanScope();

  GLint location = args[0]->Int32Value();
  GLboolean transpose = args[1]->BooleanValue();

  GLsizei count = 0;
  GLfloat* data = getArrayData<GLfloat>(args[2],&count);

  if (count < 16) {
    NanThrowError("Not enough data for UniformMatrix4fv");
  }

  glUniformMatrix4fv(location, count / 16, transpose, data);
  NanReturnUndefined();
}
NAN_METHOD(GenerateMipmap){
  NanScope();
  NanReturnUndefined();
}


static NanUtf8String *attrNameStr;

NAN_METHOD(GetAttribLocation){
  NanScope();

  int program = args[0]->Int32Value();
  attrNameStr = new NanUtf8String(args[1]);
  char* sname;
  sname = (char*) attrNameStr;

  NanReturnValue(NanNew<Number>(glGetAttribLocation(program, sname)));
}

/*GLenum func*/
NAN_METHOD(DepthFunc){
  NanScope();
  glDepthFunc(args[0]->Int32Value());
  NanReturnUndefined();
}

/*GLint x, GLint y, GLsizei width, GLsizei height*/
NAN_METHOD(Viewport){
  NanScope();
  int x = args[0]->Int32Value();
  int y = args[1]->Int32Value();
  int width = args[2]->Int32Value();
  int height = args[3]->Int32Value();
  glViewport(x, y, width, height);
  NanReturnUndefined();
}

/*GLenum shaderType*/
NAN_METHOD(CreateShader){
  NanScope();

  int shaderType = args[0]->Int32Value();
  GLuint shader = glCreateShader(shaderType);
  registerGLObj(GLOBJECT_TYPE_SHADER, shader);
  NanReturnValue(NanNew<Number>(shader));
}
/*vGLuint shader,
 	GLsizei count,
 	const GLchar * const *string,
 	const GLint *length*/
NAN_METHOD(ShaderSource){
  NanScope();

  int shader = args[0]->Int32Value();
  String::Utf8Value code(args[1]);

  const char* codes[1];
  codes[0] = *code;
  GLint length = code.length();

  glShaderSource(shader, 1, codes, &length);
  NanReturnUndefined();
}

/*GLuint shader*/
NAN_METHOD(CompileShader){
  NanScope();
  GLuint shader = args[0]->Int32Value();
  glCompileShader(shader);
  NanReturnUndefined();
}

/*GLuint shader, GLenum pname, GLint *params*/
NAN_METHOD(GetShaderParameter){
  NanScope();
  GLuint shader = args[0]->Int32Value();
  GLenum pname = args[1]->Int32Value();
  int value = 0;
  switch (pname) {
      case GL_DELETE_STATUS:
      case GL_COMPILE_STATUS:
        glGetShaderiv(shader, pname, &value);
        NanReturnValue(NanNew<Boolean>(static_cast<bool>(value!=0)));
      case GL_SHADER_TYPE:
        glGetShaderiv(shader, pname, &value);
        NanReturnValue(NanNew<Number>(static_cast<unsigned long>(value)));
      case GL_INFO_LOG_LENGTH:
      case GL_SHADER_SOURCE_LENGTH:
        glGetShaderiv(shader, pname, &value);
        NanReturnValue(NanNew<Number>(static_cast<long>(value)));
      default:
        NanThrowTypeError("GetShaderParameter: Invalid Enum");
  }

  NanReturnUndefined();
}

/*GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog*/
NAN_METHOD(GetShaderInfoLog){
  NanScope();

  int id = args[0]->Int32Value();
  int Len = 1024;
  char Error[1024];
  glGetShaderInfoLog(id, 1024, &Len, Error);

  NanReturnValue(NanNew<String>(Error));
}

NAN_METHOD(CreateProgram){
  NanScope();
  GLuint program = glCreateProgram();
  registerGLObj(GLOBJECT_TYPE_PROGRAM, program);
  NanReturnValue(NanNew<Number>(program));
}

/*	GLuint program, GLuint shader*/
NAN_METHOD(AttachShader){
  NanScope();

  int program = args[0]->Int32Value();
  int shader = args[1]->Int32Value();

  glAttachShader(program, shader);
  NanReturnUndefined();
}

/*GLuint program*/
NAN_METHOD(LinkProgram){
  NanScope();
  int program = args[0]->Int32Value();
  glLinkProgram(program);

  NanReturnUndefined();
}

/*GLuint program, GLenum pname, GLint *params*/
NAN_METHOD(GetProgramParameter){
  NanScope();
  int program = args[0]->Int32Value();
  int pname = args[1]->Int32Value();

  int value = 0;
  switch (pname) {
  case GL_DELETE_STATUS:
  case GL_LINK_STATUS:
  case GL_VALIDATE_STATUS:
    glGetProgramiv(program, pname, &value);
    NanReturnValue(NanNew<Boolean>(static_cast<bool>(value!=0)));
  case GL_ATTACHED_SHADERS:
  case GL_ACTIVE_ATTRIBUTES:
  case GL_ACTIVE_UNIFORMS:
    glGetProgramiv(program, pname, &value);
    NanReturnValue(NanNew<Boolean>(static_cast<long>(value)));
  default:
    NanThrowTypeError("GetProgramParameter: Invalid Enum");
  }
  NanReturnUndefined();
}

static NanUtf8String *prNameStr;
/*GLuint program, const GLchar *name*/
NAN_METHOD(GetUniformLocation){
  NanScope();

  int program = args[0]->Int32Value();
  prNameStr = new NanUtf8String(args[1]);
  char* sname;
  sname = (char*) prNameStr;

  NanReturnValue(NanNew<Number>(glGetUniformLocation(program, sname)));
}
NAN_METHOD(ClearColor){
  NanScope();
  float red = (float) args[0]->NumberValue();
  float green = (float) args[1]->NumberValue();
  float blue = (float) args[2]->NumberValue();
  float alpha = (float) args[3]->NumberValue();
  glClearColor(red, green, blue, alpha);
  NanReturnUndefined();
}
NAN_METHOD(ClearDepth){
  NanScope();
  GLclampf depth = (GLclampf) args[0]->NumberValue();
  glClearDepthf(depth);
  NanReturnUndefined();
}

/*args GLenum cap*/
NAN_METHOD(Disable){
  NanScope();
  glDisable(args[0]->Int32Value());
  NanReturnUndefined();
}

/*args GLenum cap*/
NAN_METHOD(Enable){
  NanScope();
  glEnable(args[0]->Int32Value());
  NanReturnUndefined();
}
NAN_METHOD(CreateTexture){
  NanScope();
  //registerGLObj(GLOBJECT_TYPE_TEXTURE, texture);
  NanReturnUndefined();
}
NAN_METHOD(BindTexture){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(TexImage2D){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(TexParameteri){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(TexParameterf){
  NanScope();
  NanReturnUndefined();
}

/*GLbitfield mask*/
NAN_METHOD(Clear){
  NanScope();
  glClear(args[0]->Int32Value());
  NanReturnUndefined();
}
NAN_METHOD(UseProgram){
  NanScope();

  glUseProgram(args[0]->Int32Value());
  NanReturnUndefined();
}

/*GLsizei n,  GLuint * buffers*/
NAN_METHOD(CreateBuffer){
  NanScope();
  GLuint buffers;
  /*TODO track created buffers*/
  glGenBuffers(1, &buffers);
  registerGLObj(GLOBJECT_TYPE_BUFFER, buffers);
  NanReturnValue(NanNew<Number>(buffers));
}

/*GLenum target,  GLuint buffer*/
NAN_METHOD(BindBuffer){
  NanScope();
  int target = args[0]->Int32Value();
  int buffer = args[1]->Uint32Value();
  glBindBuffer(target, buffer);
  NanReturnUndefined();
}
NAN_METHOD(CreateFramebuffer){
  NanScope();
  //registerGLObj(GLOBJECT_TYPE_FRAMEBUFFER, buffer);
  NanReturnUndefined();
}
NAN_METHOD(BindFramebuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(FramebufferTexture2D){
  NanScope();
  NanReturnUndefined();
}

/*GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage*/
NAN_METHOD(BufferData){
  NanScope();
  int target = args[0]->Int32Value();
  if(args[1]->IsObject()) {
    Local<Object> obj = Local<Object>::Cast(args[1]);
    GLenum usage = args[2]->Int32Value();

    int element_size = SizeOfArrayElementForType(obj->GetIndexedPropertiesExternalArrayDataType());
    GLsizeiptr size = obj->GetIndexedPropertiesExternalArrayDataLength() * element_size;
    void* data = obj->GetIndexedPropertiesExternalArrayData();
    glBufferData(target, size, data, usage);

  } else if(args[1]->IsNumber()) {
    GLsizeiptr size = args[1]->Uint32Value();
    GLenum usage = args[2]->Int32Value();
    glBufferData(target, size, NULL, usage);
  }
NanReturnUndefined();
}
NAN_METHOD(BufferSubData){
  NanScope();
  NanReturnUndefined();
}

/*GLenum mode*/
NAN_METHOD(BlendEquation){
  NanScope();
  int mode = args[0]->Int32Value();
  glBlendEquation(mode);
  NanReturnUndefined();
}

/*GLenum sfactor, GLenum dfactor*/
NAN_METHOD(BlendFunc){
  NanScope();
  glBlendFunc(args[0]->Int32Value(), args[1]->Int32Value());

  NanReturnUndefined();
}

/*GLuint index*/
NAN_METHOD(EnableVertexAttribArray){
  NanScope();

  GLuint index = args[0]->Int32Value();
  glEnableVertexAttribArray(index);
  NanReturnUndefined();
}

/*GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer*/

NAN_METHOD(VertexAttribPointer){
  NanScope();

  GLuint index = args[0]->Int32Value();
  int size = args[1]->Int32Value();
  int type = args[2]->Int32Value();
  int normalized = args[3]->BooleanValue();
  int stride = args[4]->Int32Value();
  long offset = args[5]->Int32Value();
  glVertexAttribPointer(index, size, type, normalized, stride, (const GLvoid *)offset);
  NanReturnUndefined();
}

/*GLenum mode, GLsizei count, GLenum type, const GLvoid * indices*/
NAN_METHOD(DrawElements){
  NanScope();

  int mode = args[0]->Int32Value();
  int count = args[1]->Int32Value();
  int type = args[2]->Int32Value();
  GLvoid *indices = reinterpret_cast<GLvoid*>(args[3]->Uint32Value());

  glDrawElements(mode, count, type,indices);
  NanReturnUndefined();
}
NAN_METHOD(Flush){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(Finish){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(VertexAttrib1f){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib2f){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib3f){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib4f){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib1fv){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib2fv){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib3fv){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(VertexAttrib4fv){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(BlendColor){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(BlendEquationSeparate){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(BlendFuncSeparate){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(ClearStencil){
  NanScope();
  GLint stencil = args[0]->Int32Value();
  glClearStencil(stencil);
  NanReturnUndefined();
}
/*GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha*/
NAN_METHOD(ColorMask){
  NanScope();

  GLboolean red   = args[0]->BooleanValue();
  GLboolean green = args[1]->BooleanValue();
  GLboolean blue  = args[2]->BooleanValue();
  GLboolean alpha = args[3]->BooleanValue();
  glColorMask(red, green, blue, alpha);
  NanReturnUndefined();
}
NAN_METHOD(CopyTexImage2D){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(CopyTexSubImage2D){
  NanScope();
  NanReturnUndefined();
}

/*GLenum mode*/
NAN_METHOD(CullFace){
  NanScope();
  glCullFace(args[0]->Int32Value());
  NanReturnUndefined();
}

/*args GLboolean flag*/
NAN_METHOD(DepthMask){
  NanScope();

  GLboolean flag = args[0]->BooleanValue();
  glDepthMask(flag);

  NanReturnUndefined();
}
NAN_METHOD(DepthRange){
  NanScope();
  NanReturnUndefined();
}

/*GLuint index*/
NAN_METHOD(DisableVertexAttribArray){
  NanScope();

  GLuint index = args[0]->Int32Value();
  glDisableVertexAttribArray(index);
  NanReturnUndefined();
}
NAN_METHOD(Hint){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsEnabled){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(LineWidth){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(PolygonOffset){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(Scissor){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilFunc){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilFuncSeparate){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilMask){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilMaskSeparate){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilOp){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(StencilOpSeparate){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(BindRenderbuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(CreateRenderbuffer){
  NanScope();
  //registerGLObj(GLOBJECT_TYPE_RENDERBUFFER, renderbuffers);
  NanReturnUndefined();
}

NAN_METHOD(DeleteBuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(DeleteFramebuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(DeleteProgram){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(DeleteRenderbuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(DeleteShader){
  NanScope();
  int shader = args[0]->Int32Value();
  glDeleteShader(shader);

  NanReturnUndefined();
}

NAN_METHOD(DeleteTexture){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(DetachShader){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(FramebufferRenderbuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetVertexAttribOffset){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(IsBuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsFramebuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsProgram){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsRenderbuffer){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsShader){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(IsTexture){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(RenderbufferStorage){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetShaderSource){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(ValidateProgram){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(TexSubImage2D){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(ReadPixels){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetTexParameter){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetActiveAttrib){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetActiveUniform){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetAttachedShaders){
  NanScope();
  NanReturnUndefined();
}

/*TODO implement correctly*/
NAN_METHOD(GetParameter){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetBufferParameter){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetFramebufferAttachmentParameter){
  NanScope();
  NanReturnUndefined();
}

/*GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog*/
NAN_METHOD(GetProgramInfoLog){
  NanScope();
  int id = args[0]->Int32Value();
  int Len = 1024;
  char Error[1024];
  glGetProgramInfoLog(id, 1024, &Len, Error);

  NanReturnValue(NanNew<String>(Error));
}

NAN_METHOD(GetRenderbufferParameter){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetVertexAttrib){
  NanScope();
  NanReturnUndefined();
}
NAN_METHOD(GetSupportedExtensions){
  NanScope();
  NanReturnUndefined();
}

static NanUtf8String *scopeStr;

/*TODO GetExtension triggers end of process..*/
NAN_METHOD(GetExtension){
    NanScope();

    scopeStr = new NanUtf8String(args[0]);
    cout<<"GetExtension "<<scopeStr<<endl;
    char* sname;
    sname = (char*) scopeStr;
    char *extensions = (char*) glGetString(GL_EXTENSIONS);

    char *ext = strcasestr(extensions, sname);

    if(!ext) NanReturnUndefined();

    //NanReturnValue(NanNew<String>(ext));
    NanReturnUndefined();
}

NAN_METHOD(CheckFramebufferStatus){
  NanScope();
  NanReturnUndefined();
}

NAN_METHOD(FrontFace){
  NanScope();
  glFrontFace(args[0]->Int32Value());
  NanReturnUndefined();
}

#define SET_GL_CONSTANT(name) exports->Set(NanNew<String>(#name), NanNew<Integer>(GL_ ## name))

extern "C" {
 void init(Handle<Object> exports) {

  /* ClearBufferMask */
  SET_GL_CONSTANT(DEPTH_BUFFER_BIT);
  SET_GL_CONSTANT(STENCIL_BUFFER_BIT);
  SET_GL_CONSTANT(COLOR_BUFFER_BIT);

  /* Boolean */
  SET_GL_CONSTANT(FALSE);
  SET_GL_CONSTANT(TRUE);

  /* BeginMode */
  SET_GL_CONSTANT(POINTS);
  SET_GL_CONSTANT(LINES);
  SET_GL_CONSTANT(LINE_LOOP);
  SET_GL_CONSTANT(LINE_STRIP);
  SET_GL_CONSTANT(TRIANGLES);
  SET_GL_CONSTANT(TRIANGLE_STRIP);
  SET_GL_CONSTANT(TRIANGLE_FAN);

  /* AlphaFunction (not supported in ES20) */
  /*      GL_NEVER */
  /*      GL_LESS */
  /*      GL_EQUAL */
  /*      GL_LEQUAL */
  /*      GL_GREATER */
  /*      GL_NOTEQUAL */
  /*      GL_GEQUAL */
  /*      GL_ALWAYS */

  /* BlendingFactorDest */
  SET_GL_CONSTANT(ZERO);
  SET_GL_CONSTANT(ONE);
  SET_GL_CONSTANT(SRC_COLOR);
  SET_GL_CONSTANT(ONE_MINUS_SRC_COLOR);
  SET_GL_CONSTANT(SRC_ALPHA);
  SET_GL_CONSTANT(ONE_MINUS_SRC_ALPHA);
  SET_GL_CONSTANT(DST_ALPHA);
  SET_GL_CONSTANT(ONE_MINUS_DST_ALPHA);

  /* BlendingFactorSrc */
  /*      GL_ZERO */
  /*      GL_ONE */
  SET_GL_CONSTANT(DST_COLOR);
  SET_GL_CONSTANT(ONE_MINUS_DST_COLOR);
  SET_GL_CONSTANT(SRC_ALPHA_SATURATE);
  /*      GL_SRC_ALPHA */
  /*      GL_ONE_MINUS_SRC_ALPHA */
  /*      GL_DST_ALPHA */
  /*      GL_ONE_MINUS_DST_ALPHA */

  /* BlendEquationSeparate */
  SET_GL_CONSTANT(FUNC_ADD);
  SET_GL_CONSTANT(BLEND_EQUATION);
  SET_GL_CONSTANT(BLEND_EQUATION_RGB);    /* same as BLEND_EQUATION */
  SET_GL_CONSTANT(BLEND_EQUATION_ALPHA);

  /* BlendSubtract */
  SET_GL_CONSTANT(FUNC_SUBTRACT);
  SET_GL_CONSTANT(FUNC_REVERSE_SUBTRACT);

  /* Separate Blend Functions */
  SET_GL_CONSTANT(BLEND_DST_RGB);
  SET_GL_CONSTANT(BLEND_SRC_RGB);
  SET_GL_CONSTANT(BLEND_DST_ALPHA);
  SET_GL_CONSTANT(BLEND_SRC_ALPHA);
  SET_GL_CONSTANT(CONSTANT_COLOR);
  SET_GL_CONSTANT(ONE_MINUS_CONSTANT_COLOR);
  SET_GL_CONSTANT(CONSTANT_ALPHA);
  SET_GL_CONSTANT(ONE_MINUS_CONSTANT_ALPHA);
  SET_GL_CONSTANT(BLEND_COLOR);

  /* Buffer Objects */
  SET_GL_CONSTANT(ARRAY_BUFFER);
  SET_GL_CONSTANT(ELEMENT_ARRAY_BUFFER);
  SET_GL_CONSTANT(ARRAY_BUFFER_BINDING);
  SET_GL_CONSTANT(ELEMENT_ARRAY_BUFFER_BINDING);

  SET_GL_CONSTANT(STREAM_DRAW);
  SET_GL_CONSTANT(STATIC_DRAW);
  SET_GL_CONSTANT(DYNAMIC_DRAW);

  SET_GL_CONSTANT(BUFFER_SIZE);
  SET_GL_CONSTANT(BUFFER_USAGE);

  SET_GL_CONSTANT(CURRENT_VERTEX_ATTRIB);

  /* CullFaceMode */
  SET_GL_CONSTANT(FRONT);
  SET_GL_CONSTANT(BACK);
  SET_GL_CONSTANT(FRONT_AND_BACK);

  /* DepthFunction */
  /*      GL_NEVER */
  /*      GL_LESS */
  /*      GL_EQUAL */
  /*      GL_LEQUAL */
  /*      GL_GREATER */
  /*      GL_NOTEQUAL */
  /*      GL_GEQUAL */
  /*      GL_ALWAYS */

  /* EnableCap */
  SET_GL_CONSTANT(TEXTURE_2D);
  SET_GL_CONSTANT(CULL_FACE);
  SET_GL_CONSTANT(BLEND);
  SET_GL_CONSTANT(DITHER);
  SET_GL_CONSTANT(STENCIL_TEST);
  SET_GL_CONSTANT(DEPTH_TEST);
  SET_GL_CONSTANT(SCISSOR_TEST);
  SET_GL_CONSTANT(POLYGON_OFFSET_FILL);
  SET_GL_CONSTANT(SAMPLE_ALPHA_TO_COVERAGE);
  SET_GL_CONSTANT(SAMPLE_COVERAGE);

  /* ErrorCode */
  SET_GL_CONSTANT(NO_ERROR);
  SET_GL_CONSTANT(INVALID_ENUM);
  SET_GL_CONSTANT(INVALID_VALUE);
  SET_GL_CONSTANT(INVALID_OPERATION);
  SET_GL_CONSTANT(OUT_OF_MEMORY);

  /* FrontFaceDirection */
  SET_GL_CONSTANT(CW);
  SET_GL_CONSTANT(CCW);

  /* GetPName */
  SET_GL_CONSTANT(LINE_WIDTH);
  SET_GL_CONSTANT(ALIASED_POINT_SIZE_RANGE);
  SET_GL_CONSTANT(ALIASED_LINE_WIDTH_RANGE);
  SET_GL_CONSTANT(CULL_FACE_MODE);
  SET_GL_CONSTANT(FRONT_FACE);
  SET_GL_CONSTANT(DEPTH_RANGE);
  SET_GL_CONSTANT(DEPTH_WRITEMASK);
  SET_GL_CONSTANT(DEPTH_CLEAR_VALUE);
  SET_GL_CONSTANT(DEPTH_FUNC);
  SET_GL_CONSTANT(STENCIL_CLEAR_VALUE);
  SET_GL_CONSTANT(STENCIL_FUNC);
  SET_GL_CONSTANT(STENCIL_FAIL);
  SET_GL_CONSTANT(STENCIL_PASS_DEPTH_FAIL);
  SET_GL_CONSTANT(STENCIL_PASS_DEPTH_PASS);
  SET_GL_CONSTANT(STENCIL_REF);
  SET_GL_CONSTANT(STENCIL_VALUE_MASK);
  SET_GL_CONSTANT(STENCIL_WRITEMASK);
  SET_GL_CONSTANT(STENCIL_BACK_FUNC);
  SET_GL_CONSTANT(STENCIL_BACK_FAIL);
  SET_GL_CONSTANT(STENCIL_BACK_PASS_DEPTH_FAIL);
  SET_GL_CONSTANT(STENCIL_BACK_PASS_DEPTH_PASS);
  SET_GL_CONSTANT(STENCIL_BACK_REF);
  SET_GL_CONSTANT(STENCIL_BACK_VALUE_MASK);
  SET_GL_CONSTANT(STENCIL_BACK_WRITEMASK);
  SET_GL_CONSTANT(VIEWPORT);
  SET_GL_CONSTANT(SCISSOR_BOX);
  /*      GL_SCISSOR_TEST */
  SET_GL_CONSTANT(COLOR_CLEAR_VALUE);
  SET_GL_CONSTANT(COLOR_WRITEMASK);
  SET_GL_CONSTANT(UNPACK_ALIGNMENT);
  SET_GL_CONSTANT(PACK_ALIGNMENT);
  SET_GL_CONSTANT(MAX_TEXTURE_SIZE);
  SET_GL_CONSTANT(MAX_VIEWPORT_DIMS);
  SET_GL_CONSTANT(SUBPIXEL_BITS);
  SET_GL_CONSTANT(RED_BITS);
  SET_GL_CONSTANT(GREEN_BITS);
  SET_GL_CONSTANT(BLUE_BITS);
  SET_GL_CONSTANT(ALPHA_BITS);
  SET_GL_CONSTANT(DEPTH_BITS);
  SET_GL_CONSTANT(STENCIL_BITS);
  SET_GL_CONSTANT(POLYGON_OFFSET_UNITS);
  /*      GL_POLYGON_OFFSET_FILL */
  SET_GL_CONSTANT(POLYGON_OFFSET_FACTOR);
  SET_GL_CONSTANT(TEXTURE_BINDING_2D);
  SET_GL_CONSTANT(SAMPLE_BUFFERS);
  SET_GL_CONSTANT(SAMPLES);
  SET_GL_CONSTANT(SAMPLE_COVERAGE_VALUE);
  SET_GL_CONSTANT(SAMPLE_COVERAGE_INVERT);

  /* GetTextureParameter */
  /*      GL_TEXTURE_MAG_FILTER */
  /*      GL_TEXTURE_MIN_FILTER */
  /*      GL_TEXTURE_WRAP_S */
  /*      GL_TEXTURE_WRAP_T */

  SET_GL_CONSTANT(NUM_COMPRESSED_TEXTURE_FORMATS);
  SET_GL_CONSTANT(COMPRESSED_TEXTURE_FORMATS);

  /* HintMode */
  SET_GL_CONSTANT(DONT_CARE);
  SET_GL_CONSTANT(FASTEST);
  SET_GL_CONSTANT(NICEST);

  /* HintTarget */
  SET_GL_CONSTANT(GENERATE_MIPMAP_HINT);

  /* DataType */
  SET_GL_CONSTANT(BYTE);
  SET_GL_CONSTANT(UNSIGNED_BYTE);
  SET_GL_CONSTANT(SHORT);
  SET_GL_CONSTANT(UNSIGNED_SHORT);
  SET_GL_CONSTANT(INT);
  SET_GL_CONSTANT(UNSIGNED_INT);
  SET_GL_CONSTANT(FLOAT);
#ifndef __APPLE__
  SET_GL_CONSTANT(FIXED);
#endif

  /* PixelFormat */
  SET_GL_CONSTANT(DEPTH_COMPONENT);
  SET_GL_CONSTANT(ALPHA);
  SET_GL_CONSTANT(RGB);
  SET_GL_CONSTANT(RGBA);
  SET_GL_CONSTANT(LUMINANCE);
  SET_GL_CONSTANT(LUMINANCE_ALPHA);

  /* PixelType */
  /*      GL_UNSIGNED_BYTE */
  SET_GL_CONSTANT(UNSIGNED_SHORT_4_4_4_4);
  SET_GL_CONSTANT(UNSIGNED_SHORT_5_5_5_1);
  SET_GL_CONSTANT(UNSIGNED_SHORT_5_6_5);

  /* Shaders */
  SET_GL_CONSTANT(FRAGMENT_SHADER);
  SET_GL_CONSTANT(VERTEX_SHADER);
  SET_GL_CONSTANT(MAX_VERTEX_ATTRIBS);
#ifndef __APPLE__
  SET_GL_CONSTANT(MAX_VERTEX_UNIFORM_VECTORS);
  SET_GL_CONSTANT(MAX_VARYING_VECTORS);
#endif
  SET_GL_CONSTANT(MAX_COMBINED_TEXTURE_IMAGE_UNITS);
  SET_GL_CONSTANT(MAX_VERTEX_TEXTURE_IMAGE_UNITS);
  SET_GL_CONSTANT(MAX_TEXTURE_IMAGE_UNITS);
#ifndef __APPLE__
  SET_GL_CONSTANT(MAX_FRAGMENT_UNIFORM_VECTORS);
#endif
  SET_GL_CONSTANT(SHADER_TYPE);
  SET_GL_CONSTANT(DELETE_STATUS);
  SET_GL_CONSTANT(LINK_STATUS);
  SET_GL_CONSTANT(VALIDATE_STATUS);
  SET_GL_CONSTANT(ATTACHED_SHADERS);
  SET_GL_CONSTANT(ACTIVE_UNIFORMS);
  SET_GL_CONSTANT(ACTIVE_UNIFORM_MAX_LENGTH);
  SET_GL_CONSTANT(ACTIVE_ATTRIBUTES);
  SET_GL_CONSTANT(ACTIVE_ATTRIBUTE_MAX_LENGTH);
  SET_GL_CONSTANT(SHADING_LANGUAGE_VERSION);
  SET_GL_CONSTANT(CURRENT_PROGRAM);

  /* StencilFunction */
  SET_GL_CONSTANT(NEVER);
  SET_GL_CONSTANT(LESS);
  SET_GL_CONSTANT(EQUAL);
  SET_GL_CONSTANT(LEQUAL);
  SET_GL_CONSTANT(GREATER);
  SET_GL_CONSTANT(NOTEQUAL);
  SET_GL_CONSTANT(GEQUAL);
  SET_GL_CONSTANT(ALWAYS);

  /* StencilOp */
  /*      GL_ZERO */
  SET_GL_CONSTANT(KEEP);
  SET_GL_CONSTANT(REPLACE);
  SET_GL_CONSTANT(INCR);
  SET_GL_CONSTANT(DECR);
  SET_GL_CONSTANT(INVERT);
  SET_GL_CONSTANT(INCR_WRAP);
  SET_GL_CONSTANT(DECR_WRAP);

  /* StringName */
  SET_GL_CONSTANT(VENDOR);
  SET_GL_CONSTANT(RENDERER);
  SET_GL_CONSTANT(VERSION);
  SET_GL_CONSTANT(EXTENSIONS);

  /* TextureMagFilter */
  SET_GL_CONSTANT(NEAREST);
  SET_GL_CONSTANT(LINEAR);

  /* TextureMinFilter */
  /*      GL_NEAREST */
  /*      GL_LINEAR */
  SET_GL_CONSTANT(NEAREST_MIPMAP_NEAREST);
  SET_GL_CONSTANT(LINEAR_MIPMAP_NEAREST);
  SET_GL_CONSTANT(NEAREST_MIPMAP_LINEAR);
  SET_GL_CONSTANT(LINEAR_MIPMAP_LINEAR);

  /* TextureParameterName */
  SET_GL_CONSTANT(TEXTURE_MAG_FILTER);
  SET_GL_CONSTANT(TEXTURE_MIN_FILTER);
  SET_GL_CONSTANT(TEXTURE_WRAP_S);
  SET_GL_CONSTANT(TEXTURE_WRAP_T);

  /* TextureTarget */
  /*      GL_TEXTURE_2D */
  SET_GL_CONSTANT(TEXTURE);

  SET_GL_CONSTANT(TEXTURE_CUBE_MAP);
  SET_GL_CONSTANT(TEXTURE_BINDING_CUBE_MAP);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_POSITIVE_X);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_NEGATIVE_X);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_POSITIVE_Y);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_NEGATIVE_Y);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_POSITIVE_Z);
  SET_GL_CONSTANT(TEXTURE_CUBE_MAP_NEGATIVE_Z);
  SET_GL_CONSTANT(MAX_CUBE_MAP_TEXTURE_SIZE);

  /* TextureUnit */
  SET_GL_CONSTANT(TEXTURE0);
  SET_GL_CONSTANT(TEXTURE1);
  SET_GL_CONSTANT(TEXTURE2);
  SET_GL_CONSTANT(TEXTURE3);
  SET_GL_CONSTANT(TEXTURE4);
  SET_GL_CONSTANT(TEXTURE5);
  SET_GL_CONSTANT(TEXTURE6);
  SET_GL_CONSTANT(TEXTURE7);
  SET_GL_CONSTANT(TEXTURE8);
  SET_GL_CONSTANT(TEXTURE9);
  SET_GL_CONSTANT(TEXTURE10);
  SET_GL_CONSTANT(TEXTURE11);
  SET_GL_CONSTANT(TEXTURE12);
  SET_GL_CONSTANT(TEXTURE13);
  SET_GL_CONSTANT(TEXTURE14);
  SET_GL_CONSTANT(TEXTURE15);
  SET_GL_CONSTANT(TEXTURE16);
  SET_GL_CONSTANT(TEXTURE17);
  SET_GL_CONSTANT(TEXTURE18);
  SET_GL_CONSTANT(TEXTURE19);
  SET_GL_CONSTANT(TEXTURE20);
  SET_GL_CONSTANT(TEXTURE21);
  SET_GL_CONSTANT(TEXTURE22);
  SET_GL_CONSTANT(TEXTURE23);
  SET_GL_CONSTANT(TEXTURE24);
  SET_GL_CONSTANT(TEXTURE25);
  SET_GL_CONSTANT(TEXTURE26);
  SET_GL_CONSTANT(TEXTURE27);
  SET_GL_CONSTANT(TEXTURE28);
  SET_GL_CONSTANT(TEXTURE29);
  SET_GL_CONSTANT(TEXTURE30);
  SET_GL_CONSTANT(TEXTURE31);
  SET_GL_CONSTANT(ACTIVE_TEXTURE);

  /* TextureWrapMode */
  SET_GL_CONSTANT(REPEAT);
  SET_GL_CONSTANT(CLAMP_TO_EDGE);
  SET_GL_CONSTANT(MIRRORED_REPEAT);

  /* Uniform Types */
  SET_GL_CONSTANT(FLOAT_VEC2);
  SET_GL_CONSTANT(FLOAT_VEC3);
  SET_GL_CONSTANT(FLOAT_VEC4);
  SET_GL_CONSTANT(INT_VEC2);
  SET_GL_CONSTANT(INT_VEC3);
  SET_GL_CONSTANT(INT_VEC4);
  SET_GL_CONSTANT(BOOL);
  SET_GL_CONSTANT(BOOL_VEC2);
  SET_GL_CONSTANT(BOOL_VEC3);
  SET_GL_CONSTANT(BOOL_VEC4);
  SET_GL_CONSTANT(FLOAT_MAT2);
  SET_GL_CONSTANT(FLOAT_MAT3);
  SET_GL_CONSTANT(FLOAT_MAT4);
  SET_GL_CONSTANT(SAMPLER_2D);
  SET_GL_CONSTANT(SAMPLER_CUBE);

  /* Vertex Arrays */
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_ENABLED);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_SIZE);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_STRIDE);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_TYPE);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_NORMALIZED);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_POINTER);
  SET_GL_CONSTANT(VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);

  /* Read Format */
#ifndef __APPLE__
  SET_GL_CONSTANT(IMPLEMENTATION_COLOR_READ_TYPE);
  SET_GL_CONSTANT(IMPLEMENTATION_COLOR_READ_FORMAT);
#endif

  /* Shader Source */
  SET_GL_CONSTANT(COMPILE_STATUS);
  SET_GL_CONSTANT(INFO_LOG_LENGTH);
  SET_GL_CONSTANT(SHADER_SOURCE_LENGTH);
#ifndef __APPLE__
  SET_GL_CONSTANT(SHADER_COMPILER);
#endif

  /* Shader Binary */
#ifndef __APPLE__
  SET_GL_CONSTANT(SHADER_BINARY_FORMATS);
  SET_GL_CONSTANT(NUM_SHADER_BINARY_FORMATS);
#endif

  /* Shader Precision-Specified Types */
#ifndef __APPLE__
  SET_GL_CONSTANT(LOW_FLOAT);
  SET_GL_CONSTANT(MEDIUM_FLOAT);
  SET_GL_CONSTANT(HIGH_FLOAT);
  SET_GL_CONSTANT(LOW_INT);
  SET_GL_CONSTANT(MEDIUM_INT);
  SET_GL_CONSTANT(HIGH_INT);
#endif

  /* Framebuffer Object. */
  SET_GL_CONSTANT(FRAMEBUFFER);
  SET_GL_CONSTANT(RENDERBUFFER);

  SET_GL_CONSTANT(RGBA4);
  SET_GL_CONSTANT(RGB5_A1);
#ifndef __APPLE__
  //SET_GL_CONSTANT(RGB565);
#endif
  SET_GL_CONSTANT(DEPTH_COMPONENT16);
  SET_GL_CONSTANT(STENCIL_INDEX);
  SET_GL_CONSTANT(STENCIL_INDEX8);

  SET_GL_CONSTANT(RENDERBUFFER_WIDTH);
  SET_GL_CONSTANT(RENDERBUFFER_HEIGHT);
  SET_GL_CONSTANT(RENDERBUFFER_INTERNAL_FORMAT);
  SET_GL_CONSTANT(RENDERBUFFER_RED_SIZE);
  SET_GL_CONSTANT(RENDERBUFFER_GREEN_SIZE);
  SET_GL_CONSTANT(RENDERBUFFER_BLUE_SIZE);
  SET_GL_CONSTANT(RENDERBUFFER_ALPHA_SIZE);
  SET_GL_CONSTANT(RENDERBUFFER_DEPTH_SIZE);
  SET_GL_CONSTANT(RENDERBUFFER_STENCIL_SIZE);

  SET_GL_CONSTANT(FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE);
  SET_GL_CONSTANT(FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
  SET_GL_CONSTANT(FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL);
  SET_GL_CONSTANT(FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE);

  SET_GL_CONSTANT(COLOR_ATTACHMENT0);
  SET_GL_CONSTANT(DEPTH_ATTACHMENT);
  SET_GL_CONSTANT(STENCIL_ATTACHMENT);

  SET_GL_CONSTANT(NONE);

  SET_GL_CONSTANT(FRAMEBUFFER_COMPLETE);
  SET_GL_CONSTANT(FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
  SET_GL_CONSTANT(FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
#ifndef __APPLE__
  //SET_GL_CONSTANT(FRAMEBUFFER_INCOMPLETE_DIMENSIONS);
#endif
  SET_GL_CONSTANT(FRAMEBUFFER_UNSUPPORTED);

  SET_GL_CONSTANT(FRAMEBUFFER_BINDING);
  SET_GL_CONSTANT(RENDERBUFFER_BINDING);
  SET_GL_CONSTANT(MAX_RENDERBUFFER_SIZE); //not found

  SET_GL_CONSTANT(INVALID_FRAMEBUFFER_OPERATION); //not found

  NODE_SET_METHOD(exports, "hello", Method);
  NODE_SET_METHOD(exports, "activeTexture", ActiveTexture);
  NODE_SET_METHOD(exports, "disable", Disable);
  NODE_SET_METHOD(exports, "enable", Enable);
  NODE_SET_METHOD(exports, "depthMask", DepthMask);
  NODE_SET_METHOD(exports, "colorMask", ColorMask);
  NODE_SET_METHOD(exports, "getExtension", GetExtension);
  NODE_SET_METHOD(exports, "clearColor", ClearColor);
  NODE_SET_METHOD(exports, "clearDepth", ClearDepth);
  NODE_SET_METHOD(exports, "clearStencil", ClearStencil);
  NODE_SET_METHOD(exports, "depthFunc", DepthFunc);
  NODE_SET_METHOD(exports, "frontFace", FrontFace);

  NODE_SET_METHOD(exports, "cullFace", CullFace);
  NODE_SET_METHOD(exports, "blendEquation", BlendEquation);
  NODE_SET_METHOD(exports, "blendFunc", BlendFunc);
  NODE_SET_METHOD(exports, "viewport", Viewport);
  NODE_SET_METHOD(exports, "getParameter", GetParameter);
  NODE_SET_METHOD(exports, "createBuffer", CreateBuffer);
  NODE_SET_METHOD(exports, "clear", Clear);
  NODE_SET_METHOD(exports, "bindBuffer", BindBuffer);
  NODE_SET_METHOD(exports, "bufferData", BufferData);

  NODE_SET_METHOD(exports, "createProgram", CreateProgram);
  NODE_SET_METHOD(exports, "createShader", CreateShader);
  NODE_SET_METHOD(exports, "shaderSource", ShaderSource);
  NODE_SET_METHOD(exports, "compileShader", CompileShader);
  NODE_SET_METHOD(exports, "getShaderParameter", GetShaderParameter);
  NODE_SET_METHOD(exports, "getShaderInfoLog", GetShaderInfoLog);
  NODE_SET_METHOD(exports, "attachShader", AttachShader);
  NODE_SET_METHOD(exports, "linkProgram",LinkProgram);
  NODE_SET_METHOD(exports, "getProgramInfoLog", GetProgramInfoLog);
  NODE_SET_METHOD(exports, "getProgramParameter", GetProgramParameter);
  NODE_SET_METHOD(exports, "getError", GetError);
  NODE_SET_METHOD(exports, "deleteShader", DeleteShader);
  NODE_SET_METHOD(exports, "getUniformLocation", GetUniformLocation);
  NODE_SET_METHOD(exports, "getAttribLocation", GetAttribLocation);
  NODE_SET_METHOD(exports, "useProgram", UseProgram);

  NODE_SET_METHOD(exports, "Uniform1f", Uniform1f);
  NODE_SET_METHOD(exports, "Uniform2f", Uniform2f);
  NODE_SET_METHOD(exports, "Uniform3f", Uniform3f);
  NODE_SET_METHOD(exports, "Uniform4f", Uniform4f);
  NODE_SET_METHOD(exports, "Uniform1i", Uniform1i);
  NODE_SET_METHOD(exports, "Uniform2i", Uniform2i);
  NODE_SET_METHOD(exports, "Uniform3i", Uniform3i);
  NODE_SET_METHOD(exports, "Uniform4i", Uniform4i);
  NODE_SET_METHOD(exports, "Uniform1fv", Uniform1fv);
  NODE_SET_METHOD(exports, "Uniform2fv", Uniform2fv);
  NODE_SET_METHOD(exports, "Uniform3fv", Uniform3fv);
  NODE_SET_METHOD(exports, "Uniform4fv", Uniform4fv);
  NODE_SET_METHOD(exports, "Uniform1iv", Uniform1iv);
  NODE_SET_METHOD(exports, "Uniform2iv", Uniform2iv);
  NODE_SET_METHOD(exports, "Uniform3iv", Uniform3iv);
  NODE_SET_METHOD(exports, "Uniform4iv", Uniform4iv);
  NODE_SET_METHOD(exports, "uniformMatrix2fv", UniformMatrix2fv);
  NODE_SET_METHOD(exports, "uniformMatrix3fv", UniformMatrix3fv);

  NODE_SET_METHOD(exports, "uniformMatrix4fv", UniformMatrix4fv);
  NODE_SET_METHOD(exports, "enableVertexAttribArray", EnableVertexAttribArray);
  NODE_SET_METHOD(exports, "disableVertexAttribArray", DisableVertexAttribArray);
  NODE_SET_METHOD(exports, "vertexAttribPointer", VertexAttribPointer);
  NODE_SET_METHOD(exports, "drawElements", DrawElements);


  //  NODE_SET_METHOD(exports, "bindAttribLocation", BindAttribLocation);
 }
}

void registerGLObj(GLObjectType type, GLuint obj) {
  globjs.push_back(new GLObj(type,obj));
}


void unregisterGLObj(GLuint obj) {
  if(atExit) return;

  vector<GLObj*>::iterator it = globjs.begin();
  while(globjs.size() && it != globjs.end()) {
    GLObj *globj=*it;
    if(globj->obj==obj) {
      delete globj;
      globjs.erase(it);
      break;
    }
    ++it;
  }
}

void AtExit() {
  atExit=true;
  //glFinish();

  vector<GLObj*>::iterator it;

  #ifdef LOGGING
  cout<<"WebGL AtExit() called"<<endl;
  cout<<"  # objects allocated: "<<globjs.size()<<endl;
  it = globjs.begin();
  while(globjs.size() && it != globjs.end()) {
    GLObj *obj=*it;
    cout<<"[";
    switch(obj->type) {
    case GLOBJECT_TYPE_BUFFER: cout<<"buffer"; break;
    case GLOBJECT_TYPE_FRAMEBUFFER: cout<<"framebuffer"; break;
    case GLOBJECT_TYPE_PROGRAM: cout<<"program"; break;
    case GLOBJECT_TYPE_RENDERBUFFER: cout<<"renderbuffer"; break;
    case GLOBJECT_TYPE_SHADER: cout<<"shader"; break;
    case GLOBJECT_TYPE_TEXTURE: cout<<"texture"; break;
    };
    cout<<": "<<obj->obj<<"] ";
    ++it;
  }
  cout<<endl;
  #endif

  it = globjs.begin();
  while(globjs.size() && it != globjs.end()) {
    GLObj *globj=*it;
    GLuint obj=globj->obj;

    switch(globj->type) {
    case GLOBJECT_TYPE_PROGRAM:
      #ifdef LOGGING
      cout<<"  Destroying GL program "<<obj<<endl;
      #endif
      glDeleteProgram(obj);
      break;
    case GLOBJECT_TYPE_BUFFER:
      #ifdef LOGGING
      cout<<"  Destroying GL buffer "<<obj<<endl;
      #endif
      glDeleteBuffers(1,&obj);
      break;
    case GLOBJECT_TYPE_FRAMEBUFFER:
      #ifdef LOGGING
      cout<<"  Destroying GL frame buffer "<<obj<<endl;
      #endif
      glDeleteFramebuffers(1,&obj);
      break;
    case GLOBJECT_TYPE_RENDERBUFFER:
      #ifdef LOGGING
      cout<<"  Destroying GL render buffer "<<obj<<endl;
      #endif
      glDeleteRenderbuffers(1,&obj);
      break;
    case GLOBJECT_TYPE_SHADER:
      #ifdef LOGGING
      cout<<"  Destroying GL shader "<<obj<<endl;
      #endif
      glDeleteShader(obj);
      break;
    case GLOBJECT_TYPE_TEXTURE:
      #ifdef LOGGING
      cout<<"  Destroying GL texture "<<obj<<endl;
      #endif
      glDeleteTextures(1,&obj);
      break;
    default:
      #ifdef LOGGING
      cout<<"  Unknown object "<<obj<<endl;
      #endif
      break;
    }
    delete globj;
    ++it;
  }

  globjs.clear();
}

NODE_MODULE(pigl, init)
