## Misc

```mermaid
classDiagram
class VkFramebuffer
class VkColorImageView
class VkDepthImageView
class VkResolveImageView

VkFramebuffer o--  VkColorImageView
VkFramebuffer o--  VkDepthImageView
VkFramebuffer o--  VkResolveImageView

class VkRenderPass
class VkColorAttachment
class VkDepthAttachment
class VkResolveAttachment

VkRenderPass o-- VkColorAttachment
VkRenderPass o-- VkDepthAttachment
VkRenderPass o-- VkResolveAttachment

class VkSubpass
class VkColorAttachmentRef
class VkDepthAttachmentRef
class VkRolveAttachmentRef

VkSubpass o-- VkColorAttachmentRef
VkSubpass o-- VkDepthAttachmentRef
VkSubpass o-- VkRolveAttachmentRef

VkRenderPass o-- VkSubpass
VkFramebuffer *-- VkRenderPass

```



