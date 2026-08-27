#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h> //if we want to print out enum names
#define GLFW_INCLUDE_NONE //"no, do not include OpenGL"
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

//macro function to check the return value of Vulkan API calls. Print an error message and aborts the program if it fails
#define VK_CHECK(call)                                                  \
    do {                                                                \
        VkResult vk_check_result = (call);                              \
        if (vk_check_result != VK_SUCCESS) {                            \
            std::fprintf(stderr, "%s:%d: %s failed with VkResult %d\n", \
                        __FILE__, __LINE__, #call, vk_check_result);    \
            std::abort();                                               \
        }                                                               \
    } while (0)


static const uint32_t WINDOW_WIDTH = 800;
static const uint32_t WINDOW_HEIGHT = 800;

//validation layers pass debug messages to this function
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                           VkDebugUtilsMessageTypeFlagsEXT message_types,
                                           const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                           void* user_data)
{
    (void)message_severity;
    (void)message_types;
    (void)user_data;

    std::fprintf(stderr, "[validation] %s\n", callback_data->pMessage);
    return VK_FALSE;
}

//check if our physical device supports a certain device extension
static bool DeviceSupportsExtension(VkPhysicalDevice physical_device, const char* extension_name)
{
    uint32_t extension_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr));

    std::vector<VkExtensionProperties> extensions(extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extensions.data()));

    for (uint32_t i = 0; i < extension_count; ++i) {
        if (std::strcmp(extensions[i].extensionName, extension_name) == 0) {
            return true;
        }
    }

    return false;
}

//check if our Vulkan instance supports a certain instance extension
static bool InstanceSupportsExtension(const char* extension_name)
{
    uint32_t extension_count = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));

    std::vector<VkExtensionProperties> extensions(extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()));

    for (uint32_t i = 0; i < extension_count; ++i) {
        if (std::strcmp(extensions[i].extensionName, extension_name) == 0) {
            return true;
        }
    }

    return false;
}

int main()
{

    //----------------------------------------WINDOW----------------------------------------//

    //initialize glfw
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return EXIT_FAILURE; //EXIT_FAILURE resolves to 1
    }

    //disables OpenGL context creation
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const char *window_title = "P3DG-Assignment0";

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, window_title, nullptr, nullptr);

    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        return EXIT_FAILURE;
    }
    
    //---------------------------------------INSTANCE---------------------------------------//

    //glfw needs certain vulkan instance extensions to be enabled to perform core functions like creating a window surface. we query for them here
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (!glfw_extensions) {
        std::fprintf(stderr, "glfwGetRequiredInstanceExtensions failed (Vulkan not supported by GLFW on this platform)\n");
        return EXIT_FAILURE;
    }

    std::vector<const char*> instance_extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    //let's add the debug extension so that our debug messanger works
    instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    //enable validation layers that create debug messages
    const char* enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };

    //describe our debug messanger
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr,
    };

    //describe our application
    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = window_title,
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "none",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),

        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateFlags instance_create_flags = 0;

    //mac support
    if (InstanceSupportsExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instance_create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    //describe our Vulkan instance
    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_messenger_create_info,
        .flags = instance_create_flags,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = sizeof(enabled_layers) / sizeof(enabled_layers[0]),
        .ppEnabledLayerNames = enabled_layers,
        .enabledExtensionCount = (uint32_t)instance_extensions.size(),
        .ppEnabledExtensionNames = instance_extensions.data(),
    };

    //create our Vulkan instance
    VkInstance instance = VK_NULL_HANDLE;
    VkResult instance_creation_result = vkCreateInstance(&instance_create_info, nullptr, &instance);
    VK_CHECK(instance_creation_result);

    //since extension functions are not loaded in by default (after all, they are extensions), we have to query the function as a pointer in order to use it
    PFN_vkCreateDebugUtilsMessengerEXT pfn_create_debug_utils_messengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");

    //establish the debug messanger callback
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    VK_CHECK(pfn_create_debug_utils_messengerEXT(instance, &debug_messenger_create_info, nullptr, &debug_messenger));
    
    //----------------------------------------SURFACE---------------------------------------//
    
    //create the surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    
    //------------------------------------PHYSICAL DEVICE-----------------------------------//

    //simply check the number of available physical devices (GPUs), don't get their data yet
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
    if (physical_device_count == 0) {
        std::fprintf(stderr, "vkEnumeratePhysicalDevices failed (no Vulkan-capable devices found)\n");
        return EXIT_FAILURE;
    }
    
    //properly get the physical device data, look through them and find the best* one
    //"best" meaning preferably a dedicated GPU (as opposed to an integrated one on the CPU), and minimally must support version 1.3 of Vulkan 
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t queue_family_index = UINT32_MAX;

    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties physical_device_properties = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);

        if (physical_device_properties.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }

        //devices have one or more queue families that support different pipelines
        //to keep it simple, let's grab a family that supports both the graphics pipeline and glfw surfaces 
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families.data());

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (!(queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                continue;
            }

            VkBool32 present_surface_support = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, surface, &present_surface_support));

            if (present_surface_support == VK_TRUE) {
                queue_family_index = j;
                break;
            }
        }

        if (queue_family_index == UINT32_MAX) {
            continue;
        }

        if (physical_device != VK_NULL_HANDLE && physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            continue;
        }

        physical_device = physical_devices[i];

        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        std::fprintf(stderr, "No device supporting Vulkan 1.3 + present\n");
        return EXIT_FAILURE;
    }

    //let's confirm what the name of our selected physical device is...
    VkPhysicalDeviceProperties physical_device_properties = {};
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    std::printf("Selected GPU: %s\n", physical_device_properties.deviceName);

    //-------------------------------LOGICAL DEVICE AND QUEUE-------------------------------//

    //queue priority is useful for > 1 queue but still necessary for == 1, ranges from 0.0-1.0
    //(it's used for prioritizing queues when scheduling)
    float queue_priority = 1.f;

    //describe our queue
    VkDeviceQueueCreateInfo device_queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    //note that simply being on vulkan 1.3 doesn't automatically turn 1.3 features on.
    //they default to VK_FALSE, so we enable them here and pass it to the logical device creation info
    VkPhysicalDeviceVulkan13Features vulkan_1_3_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    //presenting a surface (and by extension, using a swapchain) is not core Vulkan but rather a device extension
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    //mac support
    if (DeviceSupportsExtension(physical_device, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    {
        device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }

    //describe our logical device that serves as the interface between our Vulkan instance and our physical device (GPU)
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vulkan_1_3_features,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledLayerCount = 0, //deprecated
        .ppEnabledLayerNames = nullptr, //deprecated
        .enabledExtensionCount = (uint32_t)device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = nullptr,
    };

    //create our logical device and its queues 
    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

    //retrieve the handle of the queue generated with the logical device
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queue_index = 0;
    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);

    //---------------------------------------SWAPCHAIN--------------------------------------//

    //let's check if our physical device supports the transfer op we're going to use later
    //(don't worry, it practically always is)
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));

    //VK_IMAGE_USAGE_TRANSFER_DST_BIT is the flag that signifies the image can be used as the destination for a transfer op
    if (!(surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        std::fprintf(stderr, "Surface does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT\n");
        return EXIT_FAILURE;
    }

    //query the number of supported surface (image) formats first before fetching them
    uint32_t surface_format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr));

    //...then fetch them
    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data()));

    //for our purposes, this is the format and color space we want. in case it isn't there, we grab the first one
    //at least one format is guaranteed to exist
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    for (uint32_t i = 0; i < surface_format_count; ++i) {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = surface_formats[i];
            break;
        }
    }

    //device will support up to a specific surface size, or any size (UINT32_MAX). 
    //since we presumably get to choose, let's just use the framebuffer dimensions
    VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
    if (swapchain_extent.width == UINT32_MAX) {
        int framebuffer_width = 0;
        int framebuffer_height = 0;
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        swapchain_extent.width = (uint32_t)framebuffer_width;
        swapchain_extent.height = (uint32_t)framebuffer_height;
    }

    //minImageCount can be as low as 1...
    //let's make sure there are at least two images to use in our swapchain to "swap" betweeen
    uint32_t swapchain_min_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && swapchain_min_image_count > surface_capabilities.maxImageCount) {
        swapchain_min_image_count = surface_capabilities.maxImageCount;
    }

    //describe our swapchain
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = swapchain_min_image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swapchain_extent,
        //more than 1 only used for stereoscopic rendering
        .imageArrayLayers = 1,
        //transfer dst is here bc we clear via a transfer command
        //color attachment won't be used until we have a fully-fledged pipeline
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        //"do we plan on sharing these images with other queue families?"
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        //FIFO is guaranteed to be supported
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    //create our swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain));

    //query number of images in the swapchain we just made
    uint32_t swapchain_image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr));
    
    //grab the images the swapchain made - we do not generate them ourselves
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data()));

    //----------------------------COMMAND POOL AND COMMAND BUFFER---------------------------//

    //describe our command pool
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        //this flag enables vkResetCommandBuffer which we need because we are re-recording the command buffers on each frame
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        //a command pool is bound to (& effectively owned by) one queue family.
        //its buffers can only be submitted to queues in that family
        .queueFamilyIndex = queue_family_index,
    };

    //create our command pool
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool));

    //describe the buffer(s) we are going to create
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    //allocate (create) command buffers for the pool we've made
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer));

    //--------------------------------SYNCHRONIZATION OBJECTS-------------------------------//

    //describe our semaphores
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    //two semaphores, one to signal when an image in the swapchain has become available to be drawn over (i.e., it's not currently on-screen or in queue)
    //and one to signal when we are finished drawing into a frame, thus it is ready to be queued for presentation to the screen
    VkSemaphore image_available_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_finished_semaphore = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphore));

    //--------------------------------------FRAME LOOP--------------------------------------//

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //ask for an image to draw into. the index is not guaranteed to be sequential, so never assume such
        //i.e., it may advance by more than one so don't plan around a consistent index increment
        //semaphores are device side, so the host (our CPU/C++ side) keeps moving
        //this semaphore receieves the available image signal
        uint32_t image_index = 0;
        VkResult acquire_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

        if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
            std::fprintf(stderr, "vkAcquireNextImageKHR failed with VkResult %d\n", acquire_result);
            return EXIT_FAILURE;
        }

        VK_CHECK(vkResetCommandBuffer(command_buffer, 0));

        VkCommandBufferBeginInfo command_buffer_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };

        //this command also implicitly resets the command buffer so vkResetCommandBuffer exists moreso to show intent of buffer override
        VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

        //describe which parts of the image the following operations apply to
        VkImageSubresourceRange image_subresource_range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        //when we receive an image from vkAcquireNextImageKHR, it returns in an undefined layout.
        //we need to make sure that our image is in a writable format before we put anything in it
        VkImageMemoryBarrier2 barrier_to_transfer_dst = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,

            //wait for all commands submitted before this barrier to reach this stage...
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,

            //before any commands submitted after this barrier can begin this stage...
            .dstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            
            //the NONE stage means we don't actually have to wait on any particular stage.
            //this is fine in this case because we have a semaphore that waits for the image to be required anyway

            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            
            .image = swapchain_images[image_index],
            .subresourceRange = image_subresource_range,
        };

        VkDependencyInfo dependency_info_to_transfer_dst = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier_to_transfer_dst,
        };

        //throw our barrier into the command buffer via the dependency
        vkCmdPipelineBarrier2(command_buffer, &dependency_info_to_transfer_dst);

        //cornflower blue =D
        VkClearColorValue clear_color = {.float32 = {0.3f, 0.5f, 0.8f, 1.f}};

        //number of VkImageSubresourceRange structs in &image_subresource_range
        uint32_t range_count = 1;

        //fill the swapchain image with our clear color
        vkCmdClearColorImage(command_buffer, swapchain_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, range_count, &image_subresource_range);

        //wait until we've finished clearing (overwriting) the swapchain image with our color before we execute any following commands
        VkImageMemoryBarrier2 barrier_to_present = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,

            .srcStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            
            .image = swapchain_images[image_index],
            .subresourceRange = image_subresource_range,
        };

        VkDependencyInfo dependency_info_to_present = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier_to_present,
        };
        
        //submit the barrier
        vkCmdPipelineBarrier2(command_buffer, &dependency_info_to_present);

        //finally, we tell the command buffer it can stop listening for new commands
        VK_CHECK(vkEndCommandBuffer(command_buffer));

        //now we are focused on submitting this command buffer to the queue

        //describe and assign the semaphore we signal when we hit the CLEAR stage
        //(which tells us that the image is ready to be cleared and is thus "available")
        VkSemaphoreSubmitInfo wait_semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = image_available_semaphore,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
            .deviceIndex = 0,
        };

        VkCommandBufferSubmitInfo command_buffer_submit_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = command_buffer,
            .deviceMask = 0,
        };

        //semaphore to signal when the image is ready for presentation.
        //the whole command buffer must be complete before presenting the image
        VkSemaphoreSubmitInfo signal_semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = render_finished_semaphore,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        };

        VkSubmitInfo2 submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &wait_semaphore_info,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &command_buffer_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_semaphore_info,
        };

        //submit the command buffer to the queue 
        VK_CHECK(vkQueueSubmit2(queue, 1, &submit_info, VK_NULL_HANDLE));

        //there wasn't a command in the buffer that said "now present this image to the screen"
        //here is where we say "okay, once all of that is finished, the presentation engine can display that image"

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index,
            .pResults = nullptr,
        };

        VkResult present_result = vkQueuePresentKHR(queue, &present_info);
        if (present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR) {
            std::fprintf(stderr, "vkQueuePresentKHR failed with VkResult %d\n", present_result);
            return EXIT_FAILURE;
        }

        //now we tell host to become idle until the device has completed all processes
        //i.e., we wait for everything we've put on the queue to complete execution before giving the host more work 
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    //----------------------------------------CLEANUP---------------------------------------//
    
    //let's wait for everything to finish on the host side before destroying anything
    VK_CHECK(vkDeviceWaitIdle(device));

    //destruction generally goes in reverse order of creation

    vkDestroySemaphore(device, render_finished_semaphore, nullptr);
    vkDestroySemaphore(device, image_available_semaphore, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    PFN_vkDestroyDebugUtilsMessengerEXT pfn_destroy_debug_utils_messengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    pfn_destroy_debug_utils_messengerEXT(instance, debug_messenger, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}