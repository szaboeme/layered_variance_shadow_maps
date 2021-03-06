
<div class="page">
  <div class="left"><div class="right"><div class="main">
   <h2 class="project">Layered Variance Shadow Maps</h2>

   <div class="screenshots">
    <a href="img/naive.png"><img class="screenshot" src="img/naive.png" alt="Naive shadows" width="300" height="250"></a>
    <a href="img/pcf.png"><img class="screenshot" src="img/pcf.png" alt="PCF 3x3" width="300" height="250"></a>
    <a href="img/vsm.png"><img class="screenshot" src="img/vsm.png" alt="VSM" width="300" height="250"></a>
    <a href="img/lvsm.png"><img class="screenshot" src="img/lvsm.png" alt="LVSM 16 layers with blur and bias" width="300" height="250"></a>
   </div>

   <h4>Assignment</h4>
   <p>
   Implement the shadow technique Variance Shadow Maps (VSM) and its extension, Layered Variance Shadow Maps (LVSM).
   Compare the different shadow mapping methods visually and performance-wise to basic shadow computation methods, 
   such as Percentage Closer Filtering (PCF) or naive Z-buffer shadows.
  </p>
   <h4>Implementation</h4>
  <p>This implementation is an extension of the Shadow Mapping section of the PGR2 course materials. There are separate shaders for both render passes 
  of every shadow generation method. The basic naive shadow is generated from the hardware Z-buffer with the use of the <i>textureProj</i> function. 
  For PCF computation, the most compact hardware method was used, the <i>textureProjOffset</i> function of the fragment shader.
  <br> VSM does not compute shadows from the Z-buffer, but saves two values for each fragment, the depth and the depth squared. 
  When calculating shadows, the depth textures are sampled producing two moments of the depth distribution over a texture filter region. 
  Then Chebyshev's inequality is applied to estimate the light percentage hitting the surface at a given depth. 
  <br>In LVSM, the light space is divided into layers to increase precision and to mitigate the light bleeding introduced by variance shadow mapping.
  The actual fragment depth is warped with a linear warping function, 
  then this warped depth and its square are stored in a texture. Since one layer needs only two values per fragment, 
  there are two shadow map layers stored in each texture. Picking an odd number of layers is possible, 
  but the last texture will not be fully utilized. A maximum of 16 layers is possible in the application.
  <br>For LVSM, including the texture blurring, Multiple Render Targets (MRT) are used, multiple texture units are bound to an FBO, rendering every layer in one pass. 
  The render targets are four-component RGBA32F textures. After the depth texture generation, the layer textures are optionally blurred with a Gaussian
  blur function, and mipmaps are generated. Then, in a second render pass, the scene is rendered with shadows computed by the fragment shader 
  from the previously generated textures.
  </p>
  
   <h4>Results</h4>
   <p>The VSM method suffers from light bleeding - when shadows cast by different occluders overlap, their contour is visible (left below). Employing LVSM, dividing the light 
   space into layers may reduce or completely eliminate light bleeding. This effect is visible in the second image below, where 5 uniformly placed layers completely eliminate light bleeding
   for the chosen scene view. However, to achieve softer shadows, blurring is desirable, which highlights the light bleeding effect again (second image from right).
	To mitigate the light bleeding effect, but still have softer shadow edges, a light bias correction value is used to warp and clamp the Chebyshev estimate, 
	which produces softer shadow edges with less or no light bleeding at all (right below).
	
   <div class="screenshots">
    <a href="img/vsm.png"><img class="screenshot" src="img/vsm.png" alt="Light bleeding in VSM" width="300" height="250"></a>
    <a href="img/lvsm5.png"><img class="screenshot" src="img/lvsm5.png" alt="Light bleeding eliminated with 5 layer LVSM" width="300" height="250"></a>
    <a href="img/lvsm5blur5.png"><img class="screenshot" src="img/lvsm5blur5.png" alt="LVSM with 5x5 blur" width="300" height="250"></a>
    <a href="img/lvsm.png"><img class="screenshot" src="img/lvsm.png" alt="LVSM 16 layers with blur and bias" width="300" height="250"></a>
   </div>
	The simple Z-buffer shadows have rough edges, they are visually the least appealing. The PCF method generates smooth shadows, however, the whole scene appears 
	quite blurry. It might be suitable for simulating low quality lights. Although fine-tuning the VSM parameters to achieve a satisfactory result is not as 
	straightforward as for the PCF, the resulting shadows have soft edges, but still retain the clear contours of the occluders.
   <br><br>
   The images below demonstrate the layer boundary artefact that may be present in shadow areas (non-continuous shadow, left). 
   This error is resolved by introducing a slight overlap (0.02)
   between layers into the depth warping function (smooth shadow, right).
   <div class="screenshots">
    <a href="img/layererror.png"><img class="screenshot" src="img/layererror.png" alt="Layer artefact at boundaries" width="280" height="200"></a>
    <a href="img/layeroverlap.png"><img class="screenshot" src="img/layeroverlap.png" alt="Artefact disappears with overlap between layers (overlap = 0.02)" width="280" height="200"></a>
   </div>
   <br>
   <br>The table below shows that while the naive method is fast in shadow generation and rendering, the PCF method takes longer to render after a fast 
   depth map generation pass. On the other hand, VSM takes longer to generate the depth textures, but once the textures are prepared, the render pass is as fast
	as the naive shadow method.
   This acceleration is achieved for the price of using nearly ~2x the memory (~3x for the 2048x2048 resolution) on average compared to the naive or PCF variant.
   <br>All measurements conducted under Windows 10, on an Nvidia GeForce GTX 1060 Max-Q, 16 GB RAM, with an Intel Core i5-7300HQ 2.5 GHz. 
   </p>
   
   <table class="tg">
<thead>
  <tr>
    <th class="tg-mqa1">Resolution</th>
    <th class="tg-mqa1">Naive gen. [ms]</th>
    <th class="tg-amwm">Naive test [ms]</th>
    <th class="tg-mqa1">PCF gen. [ms]</th>
    <th class="tg-amwm">PCF test [ms]</th>
    <th class="tg-7btt">VSM gen. [ms]</th>
    <th class="tg-amwm">VSM test [ms]</th>
    <th class="tg-amwm">Blur (1 layer) [ms]</th>
    <th class="tg-amwm">Blur (8 layers) [ms]</th>
    <th class="tg-amwm">Blur (16 layers) [ms]</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td class="tg-mqa1">512x512</td>
    <td class="tg-wp8o">0.03</td>
    <td class="tg-baqh">0.06</td>
    <td class="tg-wp8o">0.04</td>
    <td class="tg-baqh">0.52</td>
    <td class="tg-c3ow">0.86</td>
    <td class="tg-baqh">0.04</td>
    <td class="tg-baqh">1.59</td>
    <td class="tg-baqh">1.82</td>
    <td class="tg-baqh">2.18</td>
  </tr>
  <tr>
    <td class="tg-mqa1">1024x1024</td>
    <td class="tg-wp8o">0.56</td>
    <td class="tg-baqh">0.08</td>
    <td class="tg-wp8o">0.58</td>
    <td class="tg-baqh">0.13</td>
    <td class="tg-c3ow">1.75</td>
    <td class="tg-baqh">0.05</td>
    <td class="tg-baqh">6.29</td>
    <td class="tg-baqh">7.07</td>
    <td class="tg-baqh">8.98</td>
  </tr>
  <tr>
    <td class="tg-7btt">2048x2048</td>
    <td class="tg-c3ow">0.90</td>
    <td class="tg-baqh">0.07</td>
    <td class="tg-c3ow">0.93</td>
    <td class="tg-baqh">0.13</td>
    <td class="tg-c3ow">4.39</td>
    <td class="tg-baqh">0.07</td>
    <td class="tg-baqh">23.75</td>
    <td class="tg-baqh">27.36</td>
    <td class="tg-baqh">34.98</td>
  </tr>
</tbody>
</table>

<h4>Application controls</h4>
		[LMB] ... scene rotation
   <br> [RMB] ... magnifier movement, if enabled in the menu
   <br> [d/D] ... +/- depth map resolution
   <br> [z/Z] ... move scene along z-axis
   <br> [l/L] ... +/- light distance from the scene
   <br> [t/T] ... change shadow technique
   <br> [s] ... show/hide first depth map texture
   <br> [0/3/5/7/9] ... change Gaussian blur kernel size
   </p>