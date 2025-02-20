#include <vulkan/vulkan_core.h>
#include <iostream>
#include <cassert>
#include <vector>

#define VOLK_IMPLEMENTATION
#include "volk.h"
#include <vulkan/vulkan.h>

#include <dlfcn.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

typedef int (*PFN_XDestroyWindow)(Display*, Window);
typedef Display* (*PFN_XOpenDisplay)(_Xconst char*);
typedef Colormap (*PFN_XCreateColormap)(Display*, Window, Visual*, int);
typedef Window (*PFN_XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int,
        unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
typedef int (*PFN_XSelectInput)(Display*, Window, long);
typedef int (*PFN_XMapWindow)(Display*, Window);
typedef Atom (*PFN_XInternAtom)(Display*, _Xconst char*, Bool);
typedef int (*PFN_XNextEvent)(Display*, XEvent*);
typedef int (*PFN_XPending)(Display*);
typedef XVisualInfo* (*PFN_XGetVisualInfo)(Display*, long, XVisualInfo*, int*);
typedef int (*PFN_XCloseDisplay)(Display* /* display */
);
typedef Status (*PFN_XInitThreads)(void);
typedef int (*PFN_XFlush)(Display* /* display */
);

static PFN_XDestroyWindow cube_XDestroyWindow = NULL;
static PFN_XOpenDisplay cube_XOpenDisplay = NULL;
static PFN_XCreateColormap cube_XCreateColormap = NULL;
static PFN_XCreateWindow cube_XCreateWindow = NULL;
static PFN_XSelectInput cube_XSelectInput = NULL;
static PFN_XMapWindow cube_XMapWindow = NULL;
static PFN_XInternAtom cube_XInternAtom = NULL;
static PFN_XNextEvent cube_XNextEvent = NULL;
static PFN_XPending cube_XPending = NULL;
static PFN_XGetVisualInfo cube_XGetVisualInfo = NULL;
static PFN_XCloseDisplay cube_XCloseDisplay = NULL;
static PFN_XInitThreads cube_XInitThreads = NULL;
static PFN_XFlush cube_XFlush = NULL;

#define XDestroyWindow cube_XDestroyWindow
#define XOpenDisplay cube_XOpenDisplay
#define XCreateColormap cube_XCreateColormap
#define XCreateWindow cube_XCreateWindow
#define XSelectInput cube_XSelectInput
#define XMapWindow cube_XMapWindow
#define XInternAtom cube_XInternAtom
#define XNextEvent cube_XNextEvent
#define XPending cube_XPending
#define XGetVisualInfo cube_XGetVisualInfo
#define XCloseDisplay cube_XCloseDisplay
#define XInitThreads cube_XInitThreads
#define XFlush cube_XFlush

void* initXlib() {
    void* xlib_library = NULL;
#if defined(XLIB_LIBRARY)
    xlib_library = dlopen(XLIB_LIBRARY, RTLD_NOW | RTLD_LOCAL);
#endif
    if (NULL == xlib_library) {
        xlib_library = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    }
    if (NULL == xlib_library) {
        xlib_library = dlopen("libX11.so", RTLD_NOW | RTLD_LOCAL);
    }
    if (NULL == xlib_library) {
        return NULL;
    }

#ifdef __cplusplus
#define TYPE_CONVERSION(type) reinterpret_cast<type>
#else
#define TYPE_CONVERSION(type)
#endif

    cube_XDestroyWindow =
            TYPE_CONVERSION(PFN_XDestroyWindow)(dlsym(xlib_library, "XDestroyWindow"));
    cube_XOpenDisplay = TYPE_CONVERSION(PFN_XOpenDisplay)(dlsym(xlib_library, "XOpenDisplay"));
    cube_XCreateColormap =
            TYPE_CONVERSION(PFN_XCreateColormap)(dlsym(xlib_library, "XCreateColormap"));
    cube_XCreateWindow = TYPE_CONVERSION(PFN_XCreateWindow)(dlsym(xlib_library, "XCreateWindow"));
    cube_XSelectInput = TYPE_CONVERSION(PFN_XSelectInput)(dlsym(xlib_library, "XSelectInput"));
    cube_XMapWindow = TYPE_CONVERSION(PFN_XMapWindow)(dlsym(xlib_library, "XMapWindow"));
    cube_XInternAtom = TYPE_CONVERSION(PFN_XInternAtom)(dlsym(xlib_library, "XInternAtom"));
    cube_XNextEvent = TYPE_CONVERSION(PFN_XNextEvent)(dlsym(xlib_library, "XNextEvent"));
    cube_XPending = TYPE_CONVERSION(PFN_XPending)(dlsym(xlib_library, "XPending"));
    cube_XGetVisualInfo =
            TYPE_CONVERSION(PFN_XGetVisualInfo)(dlsym(xlib_library, "XGetVisualInfo"));
    cube_XCloseDisplay = TYPE_CONVERSION(PFN_XCloseDisplay)(dlsym(xlib_library, "XCloseDisplay"));
    cube_XInitThreads = TYPE_CONVERSION(PFN_XInitThreads)(dlsym(xlib_library, "XInitThreads"));
    cube_XFlush = TYPE_CONVERSION(PFN_XFlush)(dlsym(xlib_library, "XFlush"));

    return xlib_library;
}

struct X11Display {
    Display* display;
    Window window;
    Atom delWindowSignal;
    uint32_t width;
    uint32_t height;
};

X11Display createWindow(uint32_t width, uint32_t height) {
    const char* display_envar = getenv("DISPLAY");
    if (display_envar == nullptr || display_envar[0] == '\0') {
        printf("Environment variable DISPLAY requires a valid value.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    XInitThreads();
    auto xlib_display = XOpenDisplay(nullptr);
    long visualMask = VisualScreenMask;
    int numberOfVisuals;
    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = DefaultScreen(xlib_display);
    XVisualInfo* visualInfo =
            XGetVisualInfo(xlib_display, visualMask, &vInfoTemplate, &numberOfVisuals);

    Colormap colormap = XCreateColormap(xlib_display,
            RootWindow(xlib_display, vInfoTemplate.screen), visualInfo->visual, AllocNone);

    XSetWindowAttributes windowAttributes = {};
    windowAttributes.colormap = colormap;
    windowAttributes.background_pixel = 0xFFFFFFFF;
    windowAttributes.border_pixel = 0;
    windowAttributes.event_mask =
            KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

    auto xlib_window = XCreateWindow(xlib_display, RootWindow(xlib_display, vInfoTemplate.screen),
            0, 0, width, height, 0, visualInfo->depth, InputOutput, visualInfo->visual,
            CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);

    XSelectInput(xlib_display, xlib_window, ExposureMask | KeyPressMask);
    XMapWindow(xlib_display, xlib_window);
    XFlush(xlib_display);
    auto xlib_wm_delete_window = XInternAtom(xlib_display, "WM_DELETE_WINDOW", False);
    return {xlib_display, xlib_window, xlib_wm_delete_window, width, height};
}

void resize(uint32_t width, uint32_t height) {}

void handleEvent(Atom deleteEvent, uint32_t const width, uint32_t const height, const XEvent* event,
        bool& quit) {
    switch (event->type) {
        case ClientMessage:
            if ((Atom) event->xclient.data.l[0] == deleteEvent) {
                quit = true;
            }
            break;
        case ConfigureNotify:
            if (((int32_t) width != event->xconfigure.width) ||
                    ((int32_t) height != event->xconfigure.height)) {
                resize(width, height);
            }
            break;
        default:
            break;
    }
}

struct VulkanState {
    uint32_t frameCount = 0;
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = 0;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
};

void draw();

void loop(X11Display const& display, VulkanState& st) {
    bool quit = false;
    auto& x11display = display.display;
    while (!quit) {
        XEvent event;
        while (XPending(x11display) > 0) {
            XNextEvent(x11display, &event);
            handleEvent(display.delWindowSignal, display.width, display.height, &event, quit);
        }

        draw();
        st.frameCount++;
    }
}

void cleanup(VulkanState& st, void* xlib, X11Display& x11) {}


// ---------------- VK stuff -----------------------
void draw() {}

template<typename StructA, typename StructB>
void chainExts(StructA* a, StructB* b) {
    a->pNext = b;
}

#define CHAIN(a, b) (chainExts(&a, &b))
#define SIZE(a) static_cast<uint32_t>(a.size())
#define DATA(a) (a.data())
#define VKALLOC VK_NULL_HANDLE

template <typename T>
using InitArray = std::vector<T>;

template<typename T, typename Result, typename ... ARGS>
InitArray<T> enumerate(Result (*func)(ARGS..., uint32_t* s, T* out), ARGS... args) {
    using ntuple = std::tuple<ARGS..., uint32_t*, T*>;
    auto nargs = std::make_tuple(std::forward<ARGS>(args)..., (uint32_t*) nullptr, (T*) nullptr);
    uint32_t*& sptr = std::get<std::tuple_size_v<ntuple>-2>(nargs);
    auto& ref = std::get<std::tuple_size_v<ntuple>-1>(nargs);
    ref = nullptr;
    uint32_t realS = 0;
    sptr = &realS;
    std::apply(func, nargs);

    InitArray<T> out;
    out.resize(realS);
    ref = out.data();
    std::apply(func, nargs);

    return out;
}

#define enumeratePhysicalDevices(...) (enumerate<VkPhysicalDevice, VkResult, VkInstance>(vkEnumeratePhysicalDevices, __VA_ARGS__))
#define getPhysicalDeviceQueueFamilyProperties(...) (enumerate<VkQueueFamilyProperties, void, VkPhysicalDevice>(vkGetPhysicalDeviceQueueFamilyProperties, __VA_ARGS__))
#define enumerateDeviceExtensionProperties(...) (enumerate<VkExtensionProperties, VkResult, VkPhysicalDevice, char const*>(vkEnumerateDeviceExtensionProperties, __VA_ARGS__))

VulkanState initVk() {
    VkResult err = volkInitialize();
    assert(err == VK_SUCCESS);

    constexpr uint32_t MAJOR_VERSION = 1;
    constexpr uint32_t MINOR_VERSION = 3;
    constexpr uint32_t INVALID_VK_INDEX = 0xFFFFFFFF;

    // instance

    InitArray<char const*> ENABLED_INST_EXTS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
    };
    InitArray<char const*> ENABLED_LAYERS = {
        "VK_LAYER_KHRONOS_validation",
    };
    InitArray<VkValidationFeatureEnableEXT> VALIDATION_ENABLES = {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_MAKE_API_VERSION(0, MAJOR_VERSION, MINOR_VERSION, 0),
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = SIZE(ENABLED_LAYERS),
        .ppEnabledLayerNames = DATA(ENABLED_LAYERS),
        .enabledExtensionCount = SIZE(ENABLED_INST_EXTS),
        .ppEnabledExtensionNames = DATA(ENABLED_INST_EXTS),
    };
    VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = SIZE(VALIDATION_ENABLES),
        .pEnabledValidationFeatures = DATA(VALIDATION_ENABLES),
    };
    CHAIN(instanceCreateInfo, validationFeatures);

    VkInstance instance;
    VkResult result = vkCreateInstance(&instanceCreateInfo, VKALLOC, &instance);
    assert(result == VK_SUCCESS);

    // Physical device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = INVALID_VK_INDEX;
    for (auto& device: enumeratePhysicalDevices(instance)) {
        VkPhysicalDeviceProperties targetDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &targetDeviceProperties);

        auto const major = VK_VERSION_MAJOR(targetDeviceProperties.apiVersion);
        auto const minor = VK_VERSION_MINOR(targetDeviceProperties.apiVersion);
        if (major < MAJOR_VERSION) {
            continue;
        } else if (major == MAJOR_VERSION) {
            if (minor < MINOR_VERSION) {
                continue;
            }
        }

        // device queue family properties
        graphicsQueueFamilyIndex = INVALID_VK_INDEX;
        uint32_t index = 0;
        for (auto const& prop: getPhysicalDeviceQueueFamilyProperties(device)) {
            if (prop.queueCount != 0 && prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueFamilyIndex = index;
                break;
            }
            index++;
        }
        if (graphicsQueueFamilyIndex == INVALID_VK_INDEX) {
            continue;
        }
        auto exts = enumerateDeviceExtensionProperties(device, nullptr);
        bool const supportsSwapchain = std::any_of(exts.begin(), exts.end(), [](auto ext) {
            return !strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        });

        if (supportsSwapchain) {
            physicalDevice = device;
            break;
        }
    }

    assert(physicalDevice != VK_NULL_HANDLE);
    assert(graphicsQueueFamilyIndex != INVALID_VK_INDEX);
    float queuePriority[] = {1.0f};
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority[0],
    };

    InitArray<char const*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = SIZE(DEVICE_EXTENSIONS),
        .ppEnabledExtensionNames = DATA(DEVICE_EXTENSIONS),
    };

    VkDevice device;
    vkCreateDevice(physicalDevice, &deviceCreateInfo, VKALLOC, &device);

    VkSurfaceFormatKHR surfaceFormat = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    VkSurfaceKHR surface;
    VkXlibSurfaceCreateInfoKHR const createInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = g_x11_vk.display,
        .window = (Window) nativeWindow,
    };
    result = vkCreateXlibSurfaceKHR(instance, &createInfo, VKALLOC,
                                    (VkSurfaceKHR*) &surface);

    VkSwapchainCreateInfoKHR const createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = 
    };
    
    return {
        .instance = instance,
        .graphicsQueueFamilyIndex = graphicsQueueFamilyIndex,
        .physicalDevice = physicalDevice,
        .device = device,
    };
}

VkSwapchainKHR createSwapChain(VulkanState const& state) {
    
}

// ---------------- end VK stuff -----------------------

int main() {
    auto xlib = initXlib();
    auto x11 = createWindow(400, 400);
    VulkanState st = initVk();
    loop(x11, st);

    cleanup(st, xlib, x11);
    return 0;
}
