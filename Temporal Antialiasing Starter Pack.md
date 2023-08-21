## Temporal Antialiasing Starter Pack

https://alextardif.com/TAA.html

### Update

在许多方面，这篇帖子现在是一篇关于Emilio López关于TAA的最新视觉教程的子集，您可以在这里找到：https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/ 如果您想要全面了解TAA，并且不仅仅是实现参考，我强烈推荐您从那里开始，然后再将这个页面作为额外的资源。

### Overview


最近，我发现自己在赶上TAA方面的进展，与我之前关于尝试避免主渲染通道时的主动态抗锯齿的其他帖子有一定的关联。在浏览关于TAA的最近（或近似的）演示文稿时，我有这样的印象，即关于实现TAA的可靠文章不多（至少我没有找到），特别是对于第一次学习它并且可能觉得它很令人生畏，或者可能对如何将这些演示文稿和论文付诸实践感到困惑的人。事实是，对于任何对学习TAA感兴趣并且不愿付出大量时间投入的人来说，一个不错的TAA实现都是可以实现的。

我不打算深入探讨TAA的理论和背景，而是在移步到我认为是一种实用的通用TAA实现的基本部分之前，会提供比我更了解这方面的人的作品链接。我在这里做的一切都不是新的或原创的，只是将各种在线资源汇集到一个地方。

### Background

在这里，我将提供我认为对于我们将要实现的TAA类型至关重要的背景阅读/演示文稿。

These are worth reading before going any further in this post:

- http://advances.realtimerendering.com/s2014/#_HIGH-QUALITY_TEMPORAL_SUPERSAMPLING
- https://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in
- https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
- http://behindthepixels.io/assets/files/TemporalAA.pdf

### Velocity Buffer