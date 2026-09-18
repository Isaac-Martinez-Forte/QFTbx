<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es_ES">
<context>
    <name>About</name>
    <message>
        <source>About QFTbx</source>
        <translation>Acerca de QFTbx</translation>
    </message>
    <message>
        <source>A toolbox for the design and analysis of robust controllers with Quantitative Feedback Theory (QFT).</source>
        <translation>Una herramienta para el diseño y el análisis de controladores robustos con la teoría de la realimentación cuantitativa (QFT).</translation>
    </message>
    <message>
        <source>It walks a design through the QFT pipeline: the plant with its parametric uncertainty and the design frequencies; the templates, the value sets of the plant at each frequency; the specifications of stability and performance; the boundaries they impose on the nominal loop in the Nichols chart; and the automatic loop shaping, which finds a controller of a given structure with the least high-frequency gain and certifies it with interval arithmetic.</source>
        <translation>Recorre un diseño por las etapas de QFT: la planta con su incertidumbre paramétrica y las frecuencias de diseño; los templates, los conjuntos de valores de la planta en cada frecuencia; las especificaciones de estabilidad y de comportamiento; los boundaries que imponen al lazo nominal en el diagrama de Nichols; y el ajuste automático del lazo, que encuentra un controlador de una estructura dada con la menor ganancia de alta frecuencia y lo certifica con aritmética de intervalos.</translation>
    </message>
    <message>
        <source>Authors</source>
        <translation>Autores</translation>
    </message>
    <message>
        <source>The algorithms come from the authors&apos; work at the University of Murcia: the degree project of 2013, the master&apos;s thesis of 2014 and the doctoral thesis of 2022, with the article in the International Journal of Robust and Nonlinear Control (2021).</source>
        <translation>Los algoritmos proceden del trabajo de los autores en la Universidad de Murcia: el proyecto fin de carrera de 2013, el trabajo fin de máster de 2014 y la tesis doctoral de 2022, con el artículo en el International Journal of Robust and Nonlinear Control (2021).</translation>
    </message>
    <message>
        <source>QFTbx is software under development: some parts are incomplete or experimental, and its results should be checked before they are relied on.</source>
        <translation>QFTbx es software en desarrollo: algunas partes están incompletas o son experimentales, y sus resultados deben comprobarse antes de confiar en ellos.</translation>
    </message>
    <message>
        <source>Source code and documentation: &lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt;. The guides to building, configuring and measuring the toolbox, and the description of every algorithm with the work it comes from, are in &lt;a href=&quot;%2&quot;&gt;docs&lt;/a&gt;.</source>
        <translation>Código fuente y documentación: &lt;a href=&quot;%1&quot;&gt;%1&lt;/a&gt;. Las guías para compilar, configurar y medir la herramienta, y la descripción de cada algoritmo con el trabajo del que procede, están en &lt;a href=&quot;%2&quot;&gt;docs&lt;/a&gt;.</translation>
    </message>
    <message>
        <source>Free software under the GNU General Public License, version 3. Built with Qt, QCustomPlot, kv and pugixml.</source>
        <translation>Software libre bajo la GNU General Public License, versión 3. Construido con Qt, QCustomPlot, kv y pugixml.</translation>
    </message>
</context>
<context>
    <name>BodeViewer</name>
    <message>
        <source>Save</source>
        <translation>Guardar</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>BoundaryGridForm</name>
    <message>
        <source>Grid data</source>
        <translation>Datos de la rejilla</translation>
    </message>
    <message>
        <source>phases:</source>
        <translation>fases:</translation>
    </message>
    <message>
        <source>Number of points:</source>
        <translation>Número de puntos:</translation>
    </message>
    <message>
        <source>Magnitude:</source>
        <translation>Magnitud:</translation>
    </message>
    <message>
        <source>Infinity = </source>
        <translation>Infinito = </translation>
    </message>
    <message>
        <source>Contour</source>
        <translation>Contorno</translation>
    </message>
    <message>
        <source>Full template</source>
        <translation>Template completo</translation>
    </message>
    <message>
        <source>CUDA</source>
        <translation>CUDA</translation>
    </message>
    <message>
        <source>CPU</source>
        <translation>CPU</translation>
    </message>
    <message>
        <source>Compute</source>
        <translation>Calcular</translation>
    </message>
    <message>
        <source>The left edge of the Nichols grid, in degrees. A boundary is only computed inside the window.</source>
        <translation>El borde izquierdo de la rejilla de Nichols, en grados. Una frontera sólo se calcula dentro de la ventana.</translation>
    </message>
    <message>
        <source>The right edge of the Nichols grid, in degrees.</source>
        <translation>El borde derecho de la rejilla de Nichols, en grados.</translation>
    </message>
    <message>
        <source>How many phases the grid is cut at. More points give a finer boundary and cost proportionally more.</source>
        <translation>En cuántas fases se corta la rejilla. Más puntos dan una frontera más fina y cuestan proporcionalmente más.</translation>
    </message>
    <message>
        <source>The bottom of the Nichols grid, in dB.</source>
        <translation>El borde inferior de la rejilla de Nichols, en dB.</translation>
    </message>
    <message>
        <source>The top of the Nichols grid, in dB.</source>
        <translation>El borde superior de la rejilla de Nichols, en dB.</translation>
    </message>
    <message>
        <source>How many magnitudes the grid is cut at.</source>
        <translation>En cuántas magnitudes se corta la rejilla.</translation>
    </message>
    <message>
        <source>Sweep the CONTOUR of each template: far fewer points, and the same boundary wherever the contour closed.</source>
        <translation>Barrer el CONTORNO de cada plantilla: muchísimos menos puntos, y la misma frontera allí donde el contorno cerró.</translation>
    </message>
    <message>
        <source>Sweep the whole template. Slower, and what to use where no contour closed.</source>
        <translation>Barrer la plantilla entera. Más lento, y lo que hay que usar donde ningún contorno cerró.</translation>
    </message>
    <message>
        <source>Sweep the grid on the GPU.</source>
        <translation>Barrer la rejilla en la GPU.</translation>
    </message>
    <message>
        <source>Sweep the grid on the processor.</source>
        <translation>Barrer la rejilla en el procesador.</translation>
    </message>
    <message>
        <source>A finite number to write in place of infinity when the boundaries are exported. It takes no part in the computation; empty means none.</source>
        <translation>Un número finito que escribir en lugar del infinito al exportar las fronteras. No interviene en el cálculo; vacío significa ninguno.</translation>
    </message>
    <message>
        <source>Computes one boundary per design frequency over this grid.</source>
        <translation>Calcula una frontera por cada frecuencia de diseño sobre esta rejilla.</translation>
    </message>
</context>
<context>
    <name>BoundaryUnionViewer</name>
    <message>
        <source>Dialog</source>
        <translation>Diálogo</translation>
    </message>
    <message>
        <source>Options</source>
        <translation>Opciones</translation>
    </message>
    <message>
        <source>Save</source>
        <translation>Guardar</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>BoundaryViewer</name>
    <message>
        <source>Dialog</source>
        <translation>Diálogo</translation>
    </message>
    <message>
        <source>Options</source>
        <translation>Opciones</translation>
    </message>
    <message>
        <source>Save</source>
        <translation>Guardar</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>ControllerForm</name>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>Polynomial form</source>
        <translation>Coeficientes de polinomios</translation>
    </message>
    <message>
        <source>Free form</source>
        <translation>Formato libre</translation>
    </message>
    <message>
        <source>Controller structure</source>
        <translation>Estructura del controlador</translation>
    </message>
    <message>
        <source>Gain k, from:</source>
        <translation>Ganancia k, de:</translation>
    </message>
    <message>
        <source>to</source>
        <translation>a</translation>
    </message>
    <message>
        <source>Controller freedom</source>
        <translation>Libertad del controlador</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>Tra&amp;nsfer function</source>
        <translation>Fu&amp;nción de transferencia</translation>
    </message>
    <message>
        <source>Zeros &amp;and poles</source>
        <translation>Ceros &amp;y polos</translation>
    </message>
    <message>
        <source>&amp;k = hfgain</source>
        <translation>&amp;k = ganancia de alta frecuencia</translation>
    </message>
    <message>
        <source>k= &amp;lfgain</source>
        <translation>k = ganancia de &amp;baja frecuencia</translation>
    </message>
    <message>
        <source>A quotient of polynomials in s, written in one of the forms beside this.</source>
        <translation>Un cociente de polinomios en s, escrito en una de las formas de al lado.</translation>
    </message>
    <message>
        <source>An expression in s. Every name in it that is not s is a parameter for the search to find.</source>
        <translation>Una expresión en s. Cualquier nombre que aparezca y no sea s es un parámetro que la búsqueda tiene que encontrar.</translation>
    </message>
    <message>
        <source>Written as its roots: one factor (s + a) per zero and per pole.</source>
        <translation>Escrita por sus raíces: un factor (s + a) por cada cero y cada polo.</translation>
    </message>
    <message>
        <source>Written as the coefficients of its two polynomials, by descending power.</source>
        <translation>Escrita por los coeficientes de sus dos polinomios, en potencias decrecientes.</translation>
    </message>
    <message>
        <source>The gain multiplies the factors (s + a), so it is the gain at high frequency.</source>
        <translation>La ganancia multiplica a los factores (s + a), así que es la ganancia a alta frecuencia.</translation>
    </message>
    <message>
        <source>The gain multiplies the factors (1 + s/T), so it is the gain at low frequency, and the values entered are time constants.</source>
        <translation>La ganancia multiplica a los factores (1 + s/T), así que es la ganancia a baja frecuencia, y los valores que se escriben son constantes de tiempo.</translation>
    </message>
    <message>
        <source>The smallest gain the search may choose.</source>
        <translation>La menor ganancia que la búsqueda puede elegir.</translation>
    </message>
    <message>
        <source>The largest. A wider box is more freedom for the search and a longer search.</source>
        <translation>La mayor. Una caja más ancha es más libertad para la búsqueda y una búsqueda más larga.</translation>
    </message>
    <message>
        <source>The interval each named zero and pole may be searched in.</source>
        <translation>El intervalo en el que se busca cada cero y cada polo con nombre.</translation>
    </message>
    <message>
        <source>Reads what is written and draws the structure it understood. Pressing it again applies it to the project.</source>
        <translation>Lee lo escrito y dibuja la estructura que ha entendido. Al pulsarlo otra vez, la aplica al proyecto.</translation>
    </message>
</context>
<context>
    <name>Core</name>
    <message>
        <source>The Nichols grid needs at least two points on each axis.</source>
        <translation>La rejilla de Nichols necesita al menos dos puntos en cada eje.</translation>
    </message>
    <message>
        <source>The Nichols grid needs a non-empty phase range and magnitude range.</source>
        <translation>La rejilla de Nichols necesita un rango de fase y otro de magnitud no vacíos.</translation>
    </message>
    <message>
        <source>The tracking boundary needs both tracking specifications (T_L and T_U).</source>
        <translation>El boundary de seguimiento necesita las dos especificaciones de seguimiento (T_L y T_U).</translation>
    </message>
    <message>
        <source>The search was cancelled.</source>
        <translation>La búsqueda se canceló.</translation>
    </message>
    <message>
        <source>%1 (line %2)</source>
        <translation>%1 (línea %2)</translation>
    </message>
    <message>
        <source>%1: %2 (line %3)</source>
        <translation>%1: %2 (línea %3)</translation>
    </message>
    <message>
        <source>A frequency set needs at least one value.</source>
        <translation>Un conjunto de frecuencias necesita al menos un valor.</translation>
    </message>
    <message>
        <source>A design frequency must be a finite positive real, and %1 is not.</source>
        <translation>Una frecuencia de diseño debe ser un real positivo finito, y %1 no lo es.</translation>
    </message>
    <message>
        <source>Cannot open frequencies file: %1</source>
        <translation>No se puede abrir el fichero de frecuencias: %1</translation>
    </message>
    <message>
        <source>The frequencies file contains no valid values: %1</source>
        <translation>El fichero de frecuencias no contiene valores válidos: %1</translation>
    </message>
    <message>
        <source>The boundaries were computed over a Nichols phase window of %1 degrees ([%2, %3]), which does not cover the full range a loop phase can take (-360 to 0 degrees). Recompute the boundaries over a window of at least 360 degrees.</source>
        <translation>Los boundaries se calcularon sobre una ventana de fase de Nichols de %1 grados ([%2, %3]), que no cubre todo el rango que puede tomar la fase de un lazo (de -360 a 0 grados). Recalcule los boundaries sobre una ventana de al menos 360 grados.</translation>
    </message>
    <message>
        <source>The search asked to bisect a controller box with no uncertain parameter.</source>
        <translation>La búsqueda pidió bisecar una caja de controlador sin ningún parámetro incierto.</translation>
    </message>
    <message>
        <source>Loop shaping supports zero-pole-gain controller structures only, for now: a time-constant, polynomial or free-form controller structure is not supported yet.</source>
        <translation>El ajuste del lazo solo admite, por ahora, estructuras de controlador de ceros, polos y ganancia: una estructura de constantes de tiempo, de polinomios o de formato libre todavía no está soportada.</translation>
    </message>
    <message>
        <source>The stability check needs at least one design frequency.</source>
        <translation>La comprobación de estabilidad necesita al menos una frecuencia de diseño.</translation>
    </message>
    <message>
        <source>The search kept %1 boxes alive at once without resolving the problem. Ask for a looser epsilon accuracy, or narrow the controller search box.</source>
        <translation>La búsqueda mantuvo %1 cajas vivas a la vez sin resolver el problema. Pida una precisión épsilon menos exigente o estreche la caja de búsqueda del controlador.</translation>
    </message>
    <message>
        <source>The search asked the live list for a node when it holds none.</source>
        <translation>La búsqueda pidió un nodo a la lista viva cuando no tiene ninguno.</translation>
    </message>
    <message>
        <source>No feasible solution exists in the given search box.</source>
        <translation>No existe ninguna solución factible en la caja de búsqueda dada.</translation>
    </message>
    <message>
        <source>The ICSP loop-shaping algorithm needs a zero-pole-gain or time-constant controller structure.</source>
        <translation>El algoritmo de ajuste del lazo ICSP necesita una estructura de controlador de ceros, polos y ganancia o de constantes de tiempo.</translation>
    </message>
    <message>
        <source>The Nichols-box termination of algorithm MR (algorithms.mr-nichols-epsilon) needs a zero-pole-gain controller structure, as the other algorithms do.</source>
        <translation>La terminación por caja de Nichols del algoritmo MR (algorithms.mr-nichols-epsilon) necesita una estructura de controlador de ceros, polos y ganancia, como los demás algoritmos.</translation>
    </message>
    <message>
        <source>A background run needs something to run.</source>
        <translation>Una ejecución en segundo plano necesita algo que ejecutar.</translation>
    </message>
    <message>
        <source>the computation failed for an unknown reason</source>
        <translation>el cálculo falló por una razón desconocida</translation>
    </message>
    <message>
        <source>The boundaries need a plant and a set of design frequencies.</source>
        <translation>Los boundaries necesitan una planta y un conjunto de frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The boundaries need the specifications.</source>
        <translation>Los boundaries necesitan las especificaciones.</translation>
    </message>
    <message>
        <source>The boundaries need the templates, which have to be recomputed after the plant or the design frequencies change.</source>
        <translation>Los boundaries necesitan los templates, que hay que recalcular cuando cambian la planta o las frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The loop shaping needs a plant and a set of design frequencies.</source>
        <translation>El ajuste del lazo necesita una planta y un conjunto de frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The loop shaping needs a controller structure.</source>
        <translation>El ajuste del lazo necesita una estructura de controlador.</translation>
    </message>
    <message>
        <source>The loop shaping needs the boundaries, which have to be recomputed after the plant, the design frequencies, the specifications or the templates change.</source>
        <translation>El ajuste del lazo necesita los boundaries, que hay que recalcular cuando cambian la planta, las frecuencias de diseño, las especificaciones o los templates.</translation>
    </message>
    <message>
        <source>The templates need a plant.</source>
        <translation>Los templates necesitan una planta.</translation>
    </message>
    <message>
        <source>The templates need a set of design frequencies.</source>
        <translation>Los templates necesitan un conjunto de frecuencias de diseño.</translation>
    </message>
    <message>
        <source>There are no templates to walk a contour over.</source>
        <translation>No hay templates sobre los que recorrer un contorno.</translation>
    </message>
    <message>
        <source>settings, line %1: &quot;%2&quot; needs %3</source>
        <translation>ajustes, línea %1: &quot;%2&quot; necesita %3</translation>
    </message>
    <message>
        <source>the settings file cannot be read: %1</source>
        <translation>no se puede leer el fichero de ajustes: %1</translation>
    </message>
    <message>
        <source>settings, line %1: a section needs its closing bracket</source>
        <translation>ajustes, línea %1: a una sección le falta el corchete de cierre</translation>
    </message>
    <message>
        <source>settings, line %1: a section needs a name</source>
        <translation>ajustes, línea %1: una sección necesita un nombre</translation>
    </message>
    <message>
        <source>settings, line %1: expected &quot;key = value&quot;, found &quot;%2&quot;</source>
        <translation>ajustes, línea %1: se esperaba &quot;clave = valor&quot; y se encontró &quot;%2&quot;</translation>
    </message>
    <message>
        <source>settings, line %1: the key is missing</source>
        <translation>ajustes, línea %1: falta la clave</translation>
    </message>
    <message>
        <source>settings, line %1: &quot;%2&quot; is set more than once</source>
        <translation>ajustes, línea %1: &quot;%2&quot; se establece más de una vez</translation>
    </message>
    <message>
        <source>A constant specification needs a finite magnitude &gt; 0.</source>
        <translation>Una especificación constante necesita una magnitud finita &gt; 0.</translation>
    </message>
    <message>
        <source>A system specification needs a non-null plant.</source>
        <translation>Una especificación de tipo sistema necesita una planta no nula.</translation>
    </message>
    <message>
        <source>The %1 specification is not in use, so it has no bound.</source>
        <translation>La especificación %1 no está en uso, así que no tiene cota.</translation>
    </message>
    <message>
        <source>A specification band needs 0 &lt;= min &lt;= max, finite.</source>
        <translation>Una banda de especificación necesita 0 &lt;= mín &lt;= máx, finitos.</translation>
    </message>
    <message>
        <source>A used specification needs a plant or a constant height.</source>
        <translation>Una especificación en uso necesita una planta o una altura constante.</translation>
    </message>
    <message>
        <source>The plant expression cannot be read: %1</source>
        <translation>No se puede leer la expresión de la planta: %1</translation>
    </message>
    <message>
        <source>A plant parameter cannot be called &quot;%1&quot;: that is the Laplace variable.</source>
        <translation>Un parámetro de la planta no puede llamarse &quot;%1&quot;: es la variable de Laplace.</translation>
    </message>
    <message>
        <source>The plant expression cannot be evaluated: %1</source>
        <translation>No se puede evaluar la expresión de la planta: %1</translation>
    </message>
    <message>
        <source>FreeForm::valueAt: %1 and %2 values were given for %3 and %4 parameters</source>
        <translation>FreeForm::valueAt: se dieron %1 y %2 valores para %3 y %4 parámetros</translation>
    </message>
    <message>
        <source>the parameter &quot;%1&quot; was given two different values (%2 and %3): the same name is the same variable</source>
        <translation>al parámetro &quot;%1&quot; se le dieron dos valores distintos (%2 y %3): el mismo nombre es la misma variable</translation>
    </message>
    <message>
        <source>A parameter&apos;s %1 must be a finite number.</source>
        <translation>El %1 de un parámetro debe ser un número finito.</translation>
    </message>
    <message>
        <source>the reparametrisation of &quot;%1&quot; cannot be read: %2</source>
        <translation>no se puede leer la reparametrización de &quot;%1&quot;: %2</translation>
    </message>
    <message>
        <source>A time constant in the %1 cannot be zero: every factor is s/z + 1.</source>
        <translation>Una constante de tiempo del %1 no puede ser cero: cada factor es s/z + 1.</translation>
    </message>
    <message>
        <source>Could not compute the templates.</source>
        <translation>No se han podido calcular los templates.</translation>
    </message>
    <message>
        <source>There are no templates to compute contours from.</source>
        <translation>No hay templates de los que calcular contornos.</translation>
    </message>
    <message>
        <source>Missing epsilon values for the template contours.</source>
        <translation>Faltan los valores de épsilon para los contornos de los templates.</translation>
    </message>
    <message>
        <source>Missing sweep grid for the uncertain parameter &apos;%1&apos;.</source>
        <translation>Falta la rejilla de barrido del parámetro incierto &apos;%1&apos;.</translation>
    </message>
    <message>
        <source>The plant expression could not be evaluated at %1 rad/s: %2</source>
        <translation>No se ha podido evaluar la expresión de la planta en %1 rad/s: %2</translation>
    </message>
    <message>
        <source>The plant has infinite magnitude at the design frequencies %1 rad/s: an undamped resonance inside the uncertainty. Its template cannot be bounded or contoured. Add light damping to the resonant poles (the usual answer for the ACC&apos;90 benchmark) or move those frequencies out of the set.</source>
        <translation>La planta tiene magnitud infinita en las frecuencias de diseño %1 rad/s: una resonancia sin amortiguar dentro de la incertidumbre. Su template no se puede acotar ni contornear. Añada un amortiguamiento ligero a los polos resonantes (la solución habitual para el problema ACC&apos;90) o saque esas frecuencias del conjunto.</translation>
    </message>
    <message>
        <source>The contours need one epsilon per design frequency: %1 given for %2 frequencies.</source>
        <translation>Los contornos necesitan un épsilon por frecuencia de diseño: se dieron %1 para %2 frecuencias.</translation>
    </message>
    <message>
        <source>Could not compute the template contours.</source>
        <translation>No se han podido calcular los contornos de los templates.</translation>
    </message>
    <message>
        <source>missing &lt;%1&gt; element</source>
        <translation>falta el elemento &lt;%1&gt;</translation>
    </message>
    <message>
        <source>&lt;%1&gt; is not a number</source>
        <translation>&lt;%1&gt; no es un número</translation>
    </message>
    <message>
        <source>&lt;%1&gt; is not a boolean</source>
        <translation>&lt;%1&gt; no es un booleano</translation>
    </message>
    <message>
        <source>missing attribute &apos;%1&apos;</source>
        <translation>falta el atributo &apos;%1&apos;</translation>
    </message>
    <message>
        <source>attribute &apos;%1&apos; is not a number</source>
        <translation>el atributo &apos;%1&apos; no es un número</translation>
    </message>
    <message>
        <source>attribute &apos;%1&apos; is not an integer</source>
        <translation>el atributo &apos;%1&apos; no es un entero</translation>
    </message>
    <message>
        <source>&lt;%1&gt; holds a non-numeric token</source>
        <translation>&lt;%1&gt; contiene un elemento no numérico</translation>
    </message>
    <message>
        <source>&lt;%1&gt; holds an odd point list</source>
        <translation>&lt;%1&gt; contiene una lista de puntos impar</translation>
    </message>
    <message>
        <source>expected exactly a gain and a delay parameter</source>
        <translation>se esperaban exactamente un parámetro de ganancia y otro de retardo</translation>
    </message>
    <message>
        <source>unknown system type</source>
        <translation>tipo de sistema desconocido</translation>
    </message>
    <message>
        <source>a non-constant specification needs its plant</source>
        <translation>una especificación no constante necesita su planta</translation>
    </message>
    <message>
        <source>the frequency set has an unknown generation type</source>
        <translation>el conjunto de frecuencias tiene un tipo de generación desconocido</translation>
    </message>
    <message>
        <source>a complex vector needs real and imaginary parts</source>
        <translation>un vector complejo necesita parte real y parte imaginaria</translation>
    </message>
    <message>
        <source>real and imaginary parts differ in length</source>
        <translation>la parte real y la imaginaria tienen longitudes distintas</translation>
    </message>
    <message>
        <source>the loop-shaping section needs its controller</source>
        <translation>la sección de ajuste del lazo necesita su controlador</translation>
    </message>
    <message>
        <source>Cannot open project file: %1</source>
        <translation>No se puede abrir el fichero de proyecto: %1</translation>
    </message>
    <message>
        <source>not a QFT project file (root &lt;%1&gt;)</source>
        <translation>no es un fichero de proyecto QFT (raíz &lt;%1&gt;)</translation>
    </message>
    <message>
        <source>The project cannot be written: &lt;%1&gt; holds a value that is not a finite number.</source>
        <translation>No se puede escribir el proyecto: &lt;%1&gt; contiene un valor que no es un número finito.</translation>
    </message>
    <message>
        <source>Cannot write project file: %1</source>
        <translation>No se puede escribir el fichero de proyecto: %1</translation>
    </message>
    <message>
        <source>&quot;%1&quot; cannot be used as a parameter name: it is a function, a constant (pi, e) or the Laplace variable s of the expression grammar, or not an identifier.</source>
        <translation>&quot;%1&quot; no puede ser el nombre de un parámetro: es una función, una constante (pi, e) o la variable de Laplace s de la gramática de expresiones, o no es un identificador.</translation>
    </message>
    <message>
        <source>The project cannot take a null plant.</source>
        <translation>El proyecto no puede recibir una planta nula.</translation>
    </message>
    <message>
        <source>The project cannot take a null set of design frequencies.</source>
        <translation>El proyecto no puede recibir un conjunto nulo de frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The project cannot take an empty set of specifications.</source>
        <translation>El proyecto no puede recibir un conjunto vacío de especificaciones.</translation>
    </message>
    <message>
        <source>There are no boundaries yet.</source>
        <translation>Todavía no hay boundaries.</translation>
    </message>
    <message>
        <source>The project cannot take a null controller structure.</source>
        <translation>El proyecto no puede recibir una estructura de controlador nula.</translation>
    </message>
    <message>
        <source>A computation is running: cancel it or wait for it before changing the project.</source>
        <translation>Hay un cálculo en marcha: cancélelo o espere a que termine antes de cambiar el proyecto.</translation>
    </message>
    <message>
        <source>a setting key needs its section, as in &apos;interface.language&apos;: &apos;%1&apos;</source>
        <translation>una clave de ajuste necesita su sección, como en &apos;interface.language&apos;: &apos;%1&apos;</translation>
    </message>
    <message>
        <source>the settings file cannot be written: %1</source>
        <translation>no se puede escribir el fichero de ajustes: %1</translation>
    </message>
    <message>
        <source>Boundary columns over different phase grids cannot be intersected: %1 columns from %2 step %3 against %4 columns from %5 step %6.</source>
        <translation>No se pueden intersectar columnas de fronteras sobre rejillas de fase distintas: %1 columnas desde %2 con paso %3 frente a %4 columnas desde %5 con paso %6.</translation>
    </message>
    <message>
        <source>The specification check needs a value set for every design frequency: %1 given for %2 frequencies.</source>
        <translation>La comprobación de las especificaciones necesita una plantilla por cada frecuencia de diseño: se dan %1 para %2 frecuencias.</translation>
    </message>
    <message>
        <source>The tracking check needs both tracking specifications (T_L and T_U).</source>
        <translation>La comprobación del seguimiento necesita las dos especificaciones de seguimiento (T_L y T_U).</translation>
    </message>
    <message>
        <source>There are no templates to propose an epsilon for.</source>
        <translation>No hay plantillas para las que proponer un épsilon.</translation>
    </message>
    <message>
        <source>The contour did not close at %1 with the epsilon given. A larger epsilon or a denser template closes it; or let the whole template stand in for the contour (templates dialog, or the setting algorithms.whole-template-if-no-contour).</source>
        <translation>El contorno no cerró en %1 con el épsilon dado. Un épsilon mayor o una plantilla más densa lo cierran; o deje que la plantilla entera haga de contorno (diálogo de plantillas, o el ajuste algorithms.whole-template-if-no-contour).</translation>
    </message>
    <message>
        <source>The decibels per degree of the Nichols metric must be a finite positive number.</source>
        <translation>Los decibelios por grado de la métrica de Nichols deben ser un número positivo finito.</translation>
    </message>
    <message>
        <source>&lt;%1&gt; holds a malformed column list</source>
        <translation>&lt;%1&gt; contiene una lista de columnas mal formada</translation>
    </message>
    <message>
        <source>&lt;%1&gt; does not cover the phase grid</source>
        <translation>&lt;%1&gt; no cubre la rejilla de fase</translation>
    </message>
    <message>
        <source>unknown epsilon metric &apos;%1&apos; (complex or nichols)</source>
        <translation>métrica del épsilon desconocida «%1» (complex o nichols)</translation>
    </message>
    <message>
        <source>the decibels per degree of the epsilon metric must be a finite positive number</source>
        <translation>los decibelios por grado de la métrica del épsilon deben ser un número positivo finito</translation>
    </message>
    <message>
        <source>The stability criterion cannot place the poles of the nominal plant: its denominator is not a polynomial in s.</source>
        <translation>El criterio de estabilidad no puede situar los polos de la planta nominal: su denominador no es un polinomio en s.</translation>
    </message>
    <message>
        <source>The gain search range must be positive.</source>
        <translation>El rango de búsqueda de la ganancia debe ser positivo.</translation>
    </message>
    <message>
        <source>The plant family changes its number of right half-plane poles: %1 with the denominator values (%2) and %3 with (%4). The stability of the nominal loop only carries to a family whose members all have the same number; split the uncertainty at the crossing.</source>
        <translation>La familia de plantas cambia su número de polos en el semiplano derecho: %1 con los valores del denominador (%2) y %3 con (%4). La estabilidad del lazo nominal sólo se traslada a una familia cuyos miembros tengan todos el mismo número; divida la incertidumbre por el cruce.</translation>
    </message>
    <message>
        <source>unsupported .qft version (no version attribute; this build reads version %1)</source>
        <translation>versión de .qft no admitida (no tiene atributo de versión; esta compilación lee la versión %1)</translation>
    </message>
    <message>
        <source>unsupported .qft version (found %1, this build reads version %2)</source>
        <translation>versión de .qft no admitida (encontrada la %1; esta compilación lee la versión %2)</translation>
    </message>
    <message>
        <source>unknown loop-shaping algorithm &apos;%1&apos;</source>
        <translation>algoritmo de ajuste del lazo desconocido «%1»</translation>
    </message>
    <message>
        <source>interface.theme must be system, light or dark: &apos;%1&apos;</source>
        <translation>interface.theme debe ser system, light o dark: «%1»</translation>
    </message>
    <message>
        <source>The computation has not finished yet.</source>
        <translation>El cálculo todavía no ha terminado.</translation>
    </message>
</context>
<context>
    <name>FrequenciesForm</name>
    <message>
        <source>Enter vector manually</source>
        <translation>Introducir el vector manualmente</translation>
    </message>
    <message>
        <source>logspace (a,b,c)</source>
        <translation>logspace (a,b,c)</translation>
    </message>
    <message>
        <source>linspace (a,b,c)</source>
        <translation>linspace (a,b,c)</translation>
    </message>
    <message>
        <source>Load vector from filePath</source>
        <translation>Cargar el vector desde un fichero</translation>
    </message>
    <message>
        <source>Enter numbers separated by spaces:</source>
        <translation>Introduzca los números separados por espacios:</translation>
    </message>
    <message>
        <source>Start (rad/s):</source>
        <translation>Inicio (rad/s):</translation>
    </message>
    <message>
        <source>End (rad/s):</source>
        <translation>Fin (rad/s):</translation>
    </message>
    <message>
        <source>n:</source>
        <translation>n:</translation>
    </message>
    <message>
        <source>Choose filePath</source>
        <translation>Seleccionar fichero</translation>
    </message>
    <message>
        <source>Input filePath:</source>
        <translation>Fichero de entrada:</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>How the design frequencies are given: typed one by one, spaced between two ends, or read from a file.</source>
        <translation>Cómo se dan las frecuencias de diseño: escritas una a una, repartidas entre dos extremos, o leídas de un fichero.</translation>
    </message>
    <message>
        <source>The frequencies themselves, in rad/s, separated by spaces. Everything downstream is computed AT them, so a handful chosen where the design is decided is worth more than a dense sweep.</source>
        <translation>Las frecuencias, en rad/s, separadas por espacios. Todo lo que viene después se calcula EN ellas, así que un puñado bien elegido donde se decide el diseño vale más que un barrido denso.</translation>
    </message>
    <message>
        <source>The first design frequency, in rad/s.</source>
        <translation>La primera frecuencia de diseño, en rad/s.</translation>
    </message>
    <message>
        <source>The last design frequency, in rad/s.</source>
        <translation>La última frecuencia de diseño, en rad/s.</translation>
    </message>
    <message>
        <source>How many frequencies between the two ends, the ends included.</source>
        <translation>Cuántas frecuencias entre los dos extremos, extremos incluidos.</translation>
    </message>
    <message>
        <source>Reads the frequencies from a text file: the numbers in it, separated by spaces or line breaks.</source>
        <translation>Lee las frecuencias de un fichero de texto: los números que haya, separados por espacios o saltos de línea.</translation>
    </message>
    <message>
        <source>Applies the frequencies. Changing them throws away the templates and everything computed from them.</source>
        <translation>Aplica las frecuencias. Cambiarlas tira las plantillas y todo lo calculado a partir de ellas.</translation>
    </message>
</context>
<context>
    <name>Language</name>
    <message>
        <source>System language</source>
        <translation>Idioma del sistema</translation>
    </message>
    <message>
        <source>English</source>
        <translation>Español</translation>
    </message>
</context>
<context>
    <name>LoopBoundariesViewer</name>
    <message>
        <source>Dialog</source>
        <translation>Diálogo</translation>
    </message>
    <message>
        <source>Options</source>
        <translation>Opciones</translation>
    </message>
    <message>
        <source>Save</source>
        <translation>Guardar</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>LoopShapingForm</name>
    <message>
        <source>Choose the algorithm:</source>
        <translation>Seleccione el algoritmo:</translation>
    </message>
    <message>
        <source>Enter epsilon:</source>
        <translation>Introduzca épsilon:</translation>
    </message>
    <message>
        <source>A&amp;lgorithm NT</source>
        <translation>A&amp;lgoritmo NT</translation>
    </message>
    <message>
        <source>Algorithm &amp;NK</source>
        <translation>Algoritmo &amp;NK</translation>
    </message>
    <message>
        <source>Algorith&amp;m MC1</source>
        <translation>Algorit&amp;mo MC1</translation>
    </message>
    <message>
        <source>Al&amp;gorithm MR</source>
        <translation>Al&amp;goritmo MR</translation>
    </message>
    <message>
        <source>Epsilon:</source>
        <translation>Épsilon:</translation>
    </message>
    <message>
        <source>Plot range:</source>
        <translation>Rango de representación:</translation>
    </message>
    <message>
        <source>Start:</source>
        <translation>Inicio:</translation>
    </message>
    <message>
        <source>End:</source>
        <translation>Fin:</translation>
    </message>
    <message>
        <source>Number of points:</source>
        <translation>Número de puntos:</translation>
    </message>
    <message>
        <source>LinSpace</source>
        <translation>LinSpace</translation>
    </message>
    <message>
        <source>LogSpace</source>
        <translation>LogSpace</translation>
    </message>
    <message>
        <source>Cen&amp;tre</source>
        <translation>Cen&amp;tro</translation>
    </message>
    <message>
        <source>Upper bo&amp;x</source>
        <translation>Caja &amp;superior</translation>
    </message>
    <message>
        <source>Initialisation type:</source>
        <translation>Tipo de inicialización:</translation>
    </message>
    <message>
        <source>Algorithm MC&amp;2</source>
        <translation>Algoritmo MC&amp;2</translation>
    </message>
    <message>
        <source>Judge every point and box by both boundary columns around its phase instead of the nearest one. Removes the small permissive error of the phase grid; the searches without a best-gain bound become much slower.</source>
        <translation>Juzgar cada punto y cada caja por las dos columnas de frontera que rodean su fase, en vez de por la más cercana. Elimina el pequeño error permisivo de la rejilla de fase; las búsquedas sin cota de mejor ganancia se vuelven mucho más lentas.</translation>
    </message>
    <message>
        <source>Conservative boundary reading</source>
        <translation>Lectura conservadora de las fronteras</translation>
    </message>
    <message>
        <source>Compute</source>
        <translation>Calcular</translation>
    </message>
    <message>
        <source>Nataraj and Tharewal: the base algorithm, a branch and bound over the box of controller parameters that PROVES its answer instead of searching for it. The slowest, and the one the other four extend.</source>
        <translation>Nataraj y Tharewal: el algoritmo base, un ramificación y poda sobre la caja de parámetros del controlador que DEMUESTRA su respuesta en vez de buscarla. El más lento, y del que salen los otros cuatro.</translation>
    </message>
    <message>
        <source>Nataraj and Kubal: NT with closed-form cuts on every parameter and a local search whose feasible results prune the tree.</source>
        <translation>Nataraj y Kubal: NT con cortes en forma cerrada sobre todos los parámetros y una búsqueda local cuyos resultados factibles podan el árbol.</translation>
    </message>
    <message>
        <source>The 2021 paper: NK with the phase and the feasible-box information added to its cuts. One to two orders of magnitude faster than NK on the published benchmarks, with the same guarantee.</source>
        <translation>El artículo de 2021: NK con la información de fase y la de caja factible añadidas a sus cortes. De uno a dos órdenes de magnitud más rápido que NK en los ejemplos publicados, con la misma garantía.</translation>
    </message>
    <message>
        <source>The strategies of the thesis with their published equations corrected, an exact best gain and a contraction of the box before it is returned.</source>
        <translation>Las estrategias de la tesis con sus ecuaciones publicadas corregidas, una mejor ganancia exacta y una contracción de la caja antes de devolverla.</translation>
    </message>
    <message>
        <source>Kalla and Nataraj: the specifications written as interval constraints on the controller parameters and solved by branch and prune. It needs no boundaries at all.</source>
        <translation>Kalla y Nataraj: las especificaciones escritas como restricciones intervalares sobre los parámetros del controlador y resueltas por ramificación y poda. No necesita fronteras.</translation>
    </message>
    <message>
        <source>The local search of NK starts at the centre of the box.</source>
        <translation>La búsqueda local de NK empieza en el centro de la caja.</translation>
    </message>
    <message>
        <source>It starts at the corner of the box: the zeros at their smallest, the poles and the gain at their largest.</source>
        <translation>Empieza en la esquina de la caja: los ceros en su valor menor, los polos y la ganancia en el mayor.</translation>
    </message>
    <message>
        <source>The first frequency the loop is DRAWN at, in rad/s. It does not change the design, which is done at the design frequencies.</source>
        <translation>La primera frecuencia a la que se DIBUJA el lazo, en rad/s. No cambia el diseño, que se hace en las frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The last frequency the loop is drawn at, in rad/s.</source>
        <translation>La última frecuencia a la que se dibuja el lazo, en rad/s.</translation>
    </message>
    <message>
        <source>How many points that drawing has.</source>
        <translation>Cuántos puntos tiene ese dibujo.</translation>
    </message>
    <message>
        <source>Those points evenly spaced.</source>
        <translation>Esos puntos repartidos por igual.</translation>
    </message>
    <message>
        <source>Spaced by decades, which is how a loop is read.</source>
        <translation>Repartidos por décadas, que es como se lee un lazo.</translation>
    </message>
    <message>
        <source>The tolerance the search stops at. It is not the same quantity for every algorithm, and the label above says which one is being asked for: the diameter of the Nichols box for NT, NK, MC1 and MC2, the width of the box of controller parameters for MR.</source>
        <translation>La tolerancia con la que la búsqueda se detiene. No es la misma magnitud para todos los algoritmos, y la etiqueta de arriba dice cuál se está pidiendo: el diámetro de la caja de Nichols para NT, NK, MC1 y MC2, y la anchura de la caja de parámetros del controlador para MR.</translation>
    </message>
    <message>
        <source>Runs the search. It can take minutes: the card says so while it runs and the search can be given up on.</source>
        <translation>Lanza la búsqueda. Puede tardar minutos: la tarjeta lo dice mientras corre y se puede abandonar.</translation>
    </message>
</context>
<context>
    <name>LoopShapingViewer</name>
    <message>
        <source>Resulting controller</source>
        <translation>Controlador resultante</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>K:</source>
        <translation>K:</translation>
    </message>
    <message>
        <source>Loop shaping</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>Save the diagram</source>
        <translation>Guardar el diagrama</translation>
    </message>
    <message>
        <source>Digits:</source>
        <translation>Cifras:</translation>
    </message>
    <message>
        <source>How many significant digits the numbers are shown at. The project file keeps every digit whatever this says.</source>
        <translation>Con cuántas cifras significativas se enseñan los números. El fichero del proyecto guarda todas las cifras diga lo que diga esto.</translation>
    </message>
    <message>
        <source>The numerator of the controller the search returned.</source>
        <translation>El numerador del controlador que ha devuelto la búsqueda.</translation>
    </message>
    <message>
        <source>Its denominator.</source>
        <translation>Su denominador.</translation>
    </message>
    <message>
        <source>Its gain.</source>
        <translation>Su ganancia.</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <source>MainWindow</source>
        <translation>MainWindow</translation>
    </message>
    <message>
        <source>Plant
definition</source>
        <translation>Definición
de la planta</translation>
    </message>
    <message>
        <source>Specifications</source>
        <translation>Especificaciones</translation>
    </message>
    <message>
        <source>Frequencies</source>
        <translation>Frecuencias</translation>
    </message>
    <message>
        <source>Templates</source>
        <translation>Templates</translation>
    </message>
    <message>
        <source>Boundaries</source>
        <translation>Boundaries</translation>
    </message>
    <message>
        <source>Loop
design</source>
        <translation>Diseño
del lazo</translation>
    </message>
    <message>
        <source>Controller
structure</source>
        <translation>Estructura
del controlador</translation>
    </message>
    <message>
        <source>&amp;File</source>
        <translation>Ar&amp;chivo</translation>
    </message>
    <message>
        <source>&amp;View</source>
        <translation>&amp;Ver</translation>
    </message>
    <message>
        <source>Dia&amp;grams</source>
        <translation>Dia&amp;gramas</translation>
    </message>
    <message>
        <source>&amp;New</source>
        <translation>&amp;Nuevo</translation>
    </message>
    <message>
        <source>&amp;Open</source>
        <translation>&amp;Abrir</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation>&amp;Guardar</translation>
    </message>
    <message>
        <source>Save &amp;as...</source>
        <translation>Guardar &amp;como...</translation>
    </message>
    <message>
        <source>&amp;Bode diagram</source>
        <translation>Diagrama de &amp;Bode</translation>
    </message>
    <message>
        <source>Nichols loop &amp;diagram</source>
        <translation>&amp;Diagrama del lazo en Nichols</translation>
    </message>
    <message>
        <source>Nyquist &amp;loop diagram</source>
        <translation>Diagrama del &amp;lazo en Nyquist</translation>
    </message>
    <message>
        <source>All &amp;diagrams</source>
        <translation>&amp;Todos los diagramas</translation>
    </message>
    <message>
        <source>&amp;Templates</source>
        <translation>&amp;Templates</translation>
    </message>
    <message>
        <source>&amp;Boundaries</source>
        <translation>&amp;Boundaries</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Lazo</translation>
    </message>
    <message>
        <source>The plant and its uncertainty. Everything else follows from it.</source>
        <translation>La planta y su incertidumbre. Todo lo demás sale de ella.</translation>
    </message>
    <message>
        <source>The frequencies the design is done at.</source>
        <translation>Las frecuencias en las que se hace el diseño.</translation>
    </message>
    <message>
        <source>What the closed loop has to do, and between which frequencies. Needs the design frequencies.</source>
        <translation>Lo que el lazo cerrado tiene que cumplir, y entre qué frecuencias. Necesita las frecuencias de diseño.</translation>
    </message>
    <message>
        <source>The value set of the plant family at each design frequency. Needs the plant and the frequencies.</source>
        <translation>El conjunto de valores de la familia de plantas en cada frecuencia de diseño. Necesita la planta y las frecuencias.</translation>
    </message>
    <message>
        <source>What the specifications and the templates leave the nominal loop, on the Nichols plane. Needs both.</source>
        <translation>Lo que las especificaciones y las plantillas le dejan al lazo nominal, en el plano de Nichols. Necesita las dos.</translation>
    </message>
    <message>
        <source>The shape of the controller and the freedom the search is given. Needs the boundaries.</source>
        <translation>La forma del controlador y la libertad que se le da a la búsqueda. Necesita los boundaries.</translation>
    </message>
    <message>
        <source>The search itself: the controller that clears every boundary. Needs the structure.</source>
        <translation>La búsqueda en sí: el controlador que respeta todas las fronteras. Necesita la estructura.</translation>
    </message>
    <message>
        <source>How far down the seven phases the project has got.</source>
        <translation>Por dónde va el proyecto en las siete fases.</translation>
    </message>
</context>
<context>
    <name>PlantForm</name>
    <message>
        <source>Tra&amp;nsfer function</source>
        <translation>Fu&amp;nción de transferencia</translation>
    </message>
    <message>
        <source>Free form</source>
        <translation>Formato libre</translation>
    </message>
    <message>
        <source>Zeros &amp;and poles</source>
        <translation>Ceros &amp;y polos</translation>
    </message>
    <message>
        <source>Polynomial form</source>
        <translation>Coeficientes de polinomios</translation>
    </message>
    <message>
        <source>&amp;k = hfgain</source>
        <translation>&amp;k = ganancia de alta frecuencia</translation>
    </message>
    <message>
        <source>k= &amp;lfgain</source>
        <translation>k = ganancia de &amp;baja frecuencia</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Ganancia:</translation>
    </message>
    <message>
        <source>Delay:</source>
        <translation>Retardo:</translation>
    </message>
    <message>
        <source>Name:</source>
        <translation>Nombre:</translation>
    </message>
    <message>
        <source>Uncertainty</source>
        <translation>Incertidumbre</translation>
    </message>
    <message>
        <source>Plant</source>
        <translation>Planta</translation>
    </message>
    <message>
        <source>Description:</source>
        <translation>Descripción:</translation>
    </message>
    <message>
        <source>What this plant is (optional)</source>
        <translation>Qué es esta planta (opcional)</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>A quotient of polynomials in s, written in one of the forms beside this.</source>
        <translation>Un cociente de polinomios en s, escrito en una de las formas de al lado.</translation>
    </message>
    <message>
        <source>An expression in s, whatever it is: a delay, a root, anything the grammar of the toolbox reads. Every name in it that is not s is an uncertain parameter.</source>
        <translation>Una expresión en s, la que sea: un retardo, una raíz, cualquier cosa que la gramática del programa sepa leer. Cualquier nombre que aparezca y no sea s es un parámetro incierto.</translation>
    </message>
    <message>
        <source>Written as its roots: one factor (s + a) per zero and per pole.</source>
        <translation>Escrita por sus raíces: un factor (s + a) por cada cero y cada polo.</translation>
    </message>
    <message>
        <source>Written as the coefficients of its two polynomials, by descending power.</source>
        <translation>Escrita por los coeficientes de sus dos polinomios, en potencias decrecientes.</translation>
    </message>
    <message>
        <source>The gain multiplies the factors (s + a), so it is the gain at high frequency.</source>
        <translation>La ganancia multiplica a los factores (s + a), así que es la ganancia a alta frecuencia.</translation>
    </message>
    <message>
        <source>The gain multiplies the factors (1 + s/T), so it is the gain at low frequency, and the values entered are time constants.</source>
        <translation>La ganancia multiplica a los factores (1 + s/T), así que es la ganancia a baja frecuencia, y los valores que se escriben son constantes de tiempo.</translation>
    </message>
    <message>
        <source>The name the plant is saved and listed under.</source>
        <translation>El nombre con el que la planta se guarda y aparece en las listas.</translation>
    </message>
    <message>
        <source>The gain, as a value. What makes it uncertain is the interval given under Uncertainty, not a name typed here.</source>
        <translation>La ganancia, como valor. Lo que la hace incierta es el intervalo que se le dé en Incertidumbre, no un nombre escrito aquí.</translation>
    </message>
    <message>
        <source>The transport delay in seconds: the plant is multiplied by e^(-delay*s). Zero if it has none.</source>
        <translation>El retardo de transporte en segundos: la planta se multiplica por e^(-retardo*s). Cero si no tiene.</translation>
    </message>
    <message>
        <source>The interval and the nominal value of every coefficient that was given a name instead of a number.</source>
        <translation>El intervalo y el valor nominal de cada coeficiente al que se le dio un nombre en vez de un número.</translation>
    </message>
    <message>
        <source>Reads what is written and draws the plant it understood. Pressing it again applies that plant to the project.</source>
        <translation>Lee lo escrito y dibuja la planta que ha entendido. Al pulsarlo otra vez, la aplica al proyecto.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>&quot;%1&quot; cannot be used as a parameter name: it is a constant of the expression grammar.</source>
        <translation>&quot;%1&quot; no puede ser el nombre de un parámetro: es una constante de la gramática de expresiones.</translation>
    </message>
    <message>
        <source>Loop-shaping input</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>QFTbx</source>
        <translation>QFTbx</translation>
    </message>
    <message>
        <source>Vector (*.pdf);;Vector (*.svg);;Image (*.png)</source>
        <translation>Vectorial (*.pdf);;Vectorial (*.svg);;Imagen (*.png)</translation>
    </message>
    <message>
        <source>Save figure</source>
        <translation>Guardar figura</translation>
    </message>
    <message>
        <source>The figure could not be saved</source>
        <translation>No se ha podido guardar la figura</translation>
    </message>
</context>
<context>
    <name>SpecificationsForm</name>
    <message>
        <source>Magnitude:</source>
        <translation>Magnitud:</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>Free form</source>
        <translation>Formato libre</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>Specifications</source>
        <translation>Especificaciones</translation>
    </message>
    <message>
        <source>Specification:</source>
        <translation>Especificación:</translation>
    </message>
    <message>
        <source>Bound:</source>
        <translation>Cota:</translation>
    </message>
    <message>
        <source>A constant</source>
        <translation>Una constante</translation>
    </message>
    <message>
        <source>A transfer function</source>
        <translation>Una función de transferencia</translation>
    </message>
    <message>
        <source>dB</source>
        <translation>dB</translation>
    </message>
    <message>
        <source>linear</source>
        <translation>lineal</translation>
    </message>
    <message>
        <source>Polynomial</source>
        <translation>Polinomios</translation>
    </message>
    <message>
        <source>Zeros and poles</source>
        <translation>Ceros y polos</translation>
    </message>
    <message>
        <source>Time constants</source>
        <translation>Constantes de tiempo</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Ganancia:</translation>
    </message>
    <message>
        <source>Delay:</source>
        <translation>Retardo:</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation>Limpiar</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>Specifications of the design</source>
        <translation>Especificaciones del diseño</translation>
    </message>
    <message>
        <source>Edit</source>
        <translation>Editar</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation>Quitar</translation>
    </message>
    <message>
        <source>Applies at:</source>
        <translation>Se aplica en:</translation>
    </message>
    <message>
        <source>Which of the seven requirements this is. Each one bounds a different closed-loop magnitude, and the formula below says which.</source>
        <translation>Cuál de las siete restricciones es ésta. Cada una acota una magnitud distinta del lazo cerrado, y la fórmula de abajo dice cuál.</translation>
    </message>
    <message>
        <source>One magnitude for the whole band.</source>
        <translation>Una sola magnitud para toda la banda.</translation>
    </message>
    <message>
        <source>A transfer function, evaluated at each frequency of the band.</source>
        <translation>Una función de transferencia, evaluada en cada frecuencia de la banda.</translation>
    </message>
    <message>
        <source>The bound itself.</source>
        <translation>La cota en sí.</translation>
    </message>
    <message>
        <source>The magnitude above is in decibels.</source>
        <translation>La magnitud de arriba está en decibelios.</translation>
    </message>
    <message>
        <source>It is a linear magnitude, where 1 is 0 dB.</source>
        <translation>Es una magnitud lineal, donde 1 son 0 dB.</translation>
    </message>
    <message>
        <source>The bound is given by the coefficients of its two polynomials, by descending power.</source>
        <translation>La cota se da por los coeficientes de sus dos polinomios, en potencias decrecientes.</translation>
    </message>
    <message>
        <source>By its zeros and poles, as factors (s + a).</source>
        <translation>Por sus ceros y polos, como factores (s + a).</translation>
    </message>
    <message>
        <source>By its time constants, as factors (1 + s/T).</source>
        <translation>Por sus constantes de tiempo, como factores (1 + s/T).</translation>
    </message>
    <message>
        <source>By an expression in s.</source>
        <translation>Por una expresión en s.</translation>
    </message>
    <message>
        <source>Separated by spaces, in the form the family above asks for.</source>
        <translation>Separados por espacios, en la forma que pide la familia de arriba.</translation>
    </message>
    <message>
        <source>The gain of the bound.</source>
        <translation>La ganancia de la cota.</translation>
    </message>
    <message>
        <source>Its delay in seconds; zero if it has none.</source>
        <translation>Su retardo en segundos; cero si no tiene.</translation>
    </message>
    <message>
        <source>Empties the fields, ready for the next specification.</source>
        <translation>Vacía los campos, listos para la siguiente especificación.</translation>
    </message>
    <message>
        <source>Reads the specification and draws it. Pressing it again puts it on the list.</source>
        <translation>Lee la especificación y la dibuja. Al pulsarlo otra vez, la pone en la lista.</translation>
    </message>
    <message>
        <source>Brings the specification chosen in the list back into the form.</source>
        <translation>Trae al formulario la especificación elegida en la lista.</translation>
    </message>
    <message>
        <source>Takes the specification chosen in the list out of the design.</source>
        <translation>Saca del diseño la especificación elegida en la lista.</translation>
    </message>
    <message>
        <source>Applies the list. The boundaries are computed from it.</source>
        <translation>Aplica la lista. Las fronteras se calculan a partir de ella.</translation>
    </message>
</context>
<context>
    <name>TemplateViewer</name>
    <message>
        <source>Templates</source>
        <translation>Templates</translation>
    </message>
    <message>
        <source>Recompute</source>
        <translation>Recalcular</translation>
    </message>
    <message>
        <source>Propose epsilon</source>
        <translation>Proponer épsilon</translation>
    </message>
    <message>
        <source>Set every epsilon to the least value at which the contour of its template closes, and recompute the contours.</source>
        <translation>Pone en cada épsilon el menor valor con el que cierra el contorno de su plantilla, y recalcula los contornos.</translation>
    </message>
    <message>
        <source>Show templates</source>
        <translation>Ver las plantillas</translation>
    </message>
    <message>
        <source>Hide contour</source>
        <translation>Ocultar el contorno</translation>
    </message>
    <message>
        <source>Save the diagram</source>
        <translation>Guardar el diagrama</translation>
    </message>
    <message>
        <source>Shows or hides the templates themselves, behind their contours.</source>
        <translation>Enseña u oculta las plantillas, detrás de sus contornos.</translation>
    </message>
    <message>
        <source>Shows or hides the contour of each template.</source>
        <translation>Enseña u oculta el contorno de cada plantilla.</translation>
    </message>
    <message>
        <source>Walks the contours again with the epsilon each frequency has in the list.</source>
        <translation>Recorre otra vez los contornos con el épsilon que cada frecuencia tiene en la lista.</translation>
    </message>
    <message>
        <source>Writes the diagram to an image file.</source>
        <translation>Guarda el diagrama en un fichero de imagen.</translation>
    </message>
</context>
<context>
    <name>TemplatesForm</name>
    <message>
        <source>LinSpace</source>
        <translation>LinSpace</translation>
    </message>
    <message>
        <source>LogSpace</source>
        <translation>LogSpace</translation>
    </message>
    <message>
        <source>Number of points:</source>
        <translation>Número de puntos:</translation>
    </message>
    <message>
        <source>Tab 1</source>
        <translation>Pestaña 1</translation>
    </message>
    <message>
        <source>Tab 2</source>
        <translation>Pestaña 2</translation>
    </message>
    <message>
        <source>Numerator</source>
        <translation>Numerador</translation>
    </message>
    <message>
        <source>Denominator</source>
        <translation>Denominador</translation>
    </message>
    <message>
        <source>All variables</source>
        <translation>Todas las variables</translation>
    </message>
    <message>
        <source>One by one</source>
        <translation>Una por una</translation>
    </message>
    <message>
        <source>Nichols diagram</source>
        <translation>Diagrama de Nichols</translation>
    </message>
    <message>
        <source>Nyquist diagram</source>
        <translation>Diagrama de Nyquist</translation>
    </message>
    <message>
        <source>Epsilon:</source>
        <translation>Épsilon:</translation>
    </message>
    <message>
        <source>One value, or one per design frequency. Filled in with the least epsilon at which the contour of each template closes, over the grids as they are entered above.</source>
        <translation>Un valor, o uno por frecuencia de diseño. Se rellena con el menor épsilon con el que cierra el contorno de cada plantilla, sobre las mallas tal como están arriba.</translation>
    </message>
    <message>
        <source>Sweep the family over the grids entered above and fill in the least epsilon at which the contour of each template closes, in the plane chosen below.</source>
        <translation>Barre la familia sobre las mallas de arriba y rellena el menor épsilon con el que cierra el contorno de cada plantilla, en el plano elegido abajo.</translation>
    </message>
    <message>
        <source>Propose</source>
        <translation>Proponer</translation>
    </message>
    <message>
        <source>Where the contour walk does not close with the epsilon given, use the whole template as its contour at that frequency: always safe, only slower, and marked in the viewer. Unchecked, the computation stops and names the frequency instead.</source>
        <translation>Donde el recorrido del contorno no cierre con el épsilon dado, usar la plantilla entera como su contorno en esa frecuencia: siempre seguro, sólo más lento, y marcado en el visor. Sin marcar, el cálculo se detiene y nombra la frecuencia.</translation>
    </message>
    <message>
        <source>Use the whole template where the contour does not close</source>
        <translation>Usar la plantilla entera donde el contorno no cierra</translation>
    </message>
    <message>
        <source>Epsilon in:</source>
        <translation>Épsilon en:</translation>
    </message>
    <message>
        <source>The plane the epsilon of the contour is measured in. Nichols: degrees and decibels, one epsilon serves every template. Complex plane: the historical reading, in the units of the plant&apos;s response.</source>
        <translation>El plano en que se mide el épsilon del contorno. Nichols: grados y decibelios, un solo épsilon sirve para todas las plantillas. Plano complejo: la lectura histórica, en las unidades de la respuesta de la planta.</translation>
    </message>
    <message>
        <source>Nichols (degrees, dB)</source>
        <translation>Nichols (grados, dB)</translation>
    </message>
    <message>
        <source>Complex plane</source>
        <translation>Plano complejo</translation>
    </message>
    <message>
        <source>dB per degree:</source>
        <translation>dB por grado:</translation>
    </message>
    <message>
        <source>Contour:</source>
        <translation>Contorno:</translation>
    </message>
    <message>
        <source>How the contour of each template is extracted. The walk of Nordin is the historical epsilon-hull and can fail to close. The alpha-shape is the same boundary by its definition, edge by edge: it always closes and returns every component and hole, and the proposed epsilon is then the least that keeps the template connected.</source>
        <translation>Cómo se extrae el contorno de cada plantilla. El recorrido de Nordin es el ε-hull histórico y puede no cerrar. El α-shape es la misma frontera por su definición, arista a arista: siempre cierra y devuelve todas las componentes, y el épsilon propuesto es entonces el menor que mantiene conectada la plantilla.</translation>
    </message>
    <message>
        <source>Epsilon-hull walk (Nordin)</source>
        <translation>Recorrido ε-hull (Nordin)</translation>
    </message>
    <message>
        <source>Alpha-shape (always closes)</source>
        <translation>α-shape (siempre cierra)</translation>
    </message>
    <message>
        <source>With exactly two uncertain parameters, sweep only the border of the parameter box: as many evaluations as the interior grid would cost, spent on the four edges, since the worst case of every specification over a template lies on its border. The template is then a closed curve and its contour is taken by the alpha-shape. Unavailable with one or with three or more uncertain parameters.</source>
        <translation>Con exactamente dos parámetros inciertos, barrer sólo el borde de la caja de parámetros: tantas evaluaciones como costaría la malla interior, gastadas en las cuatro aristas, porque el peor caso de toda especificación sobre una plantilla está en su borde. La plantilla es entonces una curva cerrada y su contorno lo toma el α-shape. No disponible con uno o con tres o más parámetros inciertos.</translation>
    </message>
    <message>
        <source>Sweep only the border of the parameter box (two parameters)</source>
        <translation>Barrer sólo el borde de la caja de parámetros (dos parámetros)</translation>
    </message>
    <message>
        <source>Compute</source>
        <translation>Calcular</translation>
    </message>
    <message>
        <source>On the GPU</source>
        <translation>En la GPU</translation>
    </message>
    <message>
        <source>One grid for every uncertain parameter, with the same number of values for all of them.</source>
        <translation>Una rejilla para todos los parámetros inciertos, con el mismo número de valores para todos.</translation>
    </message>
    <message>
        <source>A grid of its own for each parameter.</source>
        <translation>Una rejilla propia para cada parámetro.</translation>
    </message>
    <message>
        <source>The values of each parameter evenly spaced between its ends.</source>
        <translation>Los valores de cada parámetro repartidos por igual entre sus extremos.</translation>
    </message>
    <message>
        <source>Spaced by decades, which is what a parameter that spans orders of magnitude asks for.</source>
        <translation>Repartidos por décadas, que es lo que pide un parámetro que abarca órdenes de magnitud.</translation>
    </message>
    <message>
        <source>How many values each uncertain parameter takes. A template has one point per COMBINATION of them, so this multiplies: five parameters at ten values each are a hundred thousand points per frequency.</source>
        <translation>Cuántos valores toma cada parámetro incierto. Una plantilla tiene un punto por cada COMBINACIÓN de ellos, así que esto multiplica: cinco parámetros a diez valores cada uno son cien mil puntos por frecuencia.</translation>
    </message>
    <message>
        <source>The parameters of the numerator.</source>
        <translation>Los parámetros del numerador.</translation>
    </message>
    <message>
        <source>The parameters of the denominator.</source>
        <translation>Los parámetros del denominador.</translation>
    </message>
    <message>
        <source>Draw the templates on the Nichols plane, phase against magnitude.</source>
        <translation>Dibujar las plantillas en el plano de Nichols, fase contra magnitud.</translation>
    </message>
    <message>
        <source>Draw them on the complex plane.</source>
        <translation>Dibujarlas en el plano complejo.</translation>
    </message>
    <message>
        <source>Sweep the family on the GPU.</source>
        <translation>Barrer la familia en la GPU.</translation>
    </message>
    <message>
        <source>How many decibels one degree is worth when the epsilon is measured on the Nichols plane, which has two units on its axes.</source>
        <translation>Cuántos decibelios vale un grado cuando el épsilon se mide en el plano de Nichols, que tiene dos unidades distintas en sus ejes.</translation>
    </message>
    <message>
        <source>Sweeps the family at every design frequency: the templates and the contour of each one.</source>
        <translation>Barre la familia en cada frecuencia de diseño: las plantillas y el contorno de cada una.</translation>
    </message>
</context>
<context>
    <name>UncertaintyPanel</name>
    <message>
        <source>Uncertainty</source>
        <translation>Incertidumbre</translation>
    </message>
    <message>
        <source>Range of every uncertain coefficient</source>
        <translation>Intervalo de cada coeficiente incierto</translation>
    </message>
    <message>
        <source>No coefficient of this system is uncertain: a coefficient becomes uncertain by being given a name instead of a value.</source>
        <translation>Ningún coeficiente de este sistema es incierto: un coeficiente se vuelve incierto al darle un nombre en vez de un valor.</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Ganancia:</translation>
    </message>
    <message>
        <source>to</source>
        <translation>a</translation>
    </message>
    <message>
        <source>Delay:</source>
        <translation>Retardo:</translation>
    </message>
    <message>
        <source>Back</source>
        <translation>Volver</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>The smallest value the gain takes.</source>
        <translation>El menor valor que toma la ganancia.</translation>
    </message>
    <message>
        <source>The largest.</source>
        <translation>El mayor.</translation>
    </message>
    <message>
        <source>The smallest value the delay takes, in seconds.</source>
        <translation>El menor valor que toma el retardo, en segundos.</translation>
    </message>
    <message>
        <source>Back to the form, leaving these ranges as they were.</source>
        <translation>Volver al formulario, dejando estos intervalos como estaban.</translation>
    </message>
    <message>
        <source>Applies the ranges to the coefficients that were named.</source>
        <translation>Aplica los intervalos a los coeficientes que llevan nombre.</translation>
    </message>
</context>
<context>
    <name>qftbx::Application</name>
    <message>
        <source>QFTbx</source>
        <translation>QFTbx</translation>
    </message>
</context>
<context>
    <name>qftbx::BodeViewer</name>
    <message>
        <source>Bode diagram</source>
        <translation>Diagrama de Bode</translation>
    </message>
    <message>
        <source>Magnitude (dB)</source>
        <translation>Magnitud (dB)</translation>
    </message>
    <message>
        <source>Phase (deg)</source>
        <translation>Fase (grados)</translation>
    </message>
    <message>
        <source>frequency (rad/s)</source>
        <translation>frecuencia (rad/s)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>Save figure</source>
        <translation>Guardar figura</translation>
    </message>
    <message>
        <source>The figure could not be saved</source>
        <translation>No se ha podido guardar la figura</translation>
    </message>
</context>
<context>
    <name>qftbx::BoundaryGridForm</name>
    <message>
        <source>Boundary grid input</source>
        <translation>Rejilla de los boundaries</translation>
    </message>
    <message>
        <source>The grid ranges must be increasing, with at least 2 points per axis.</source>
        <translation>Los rangos de la rejilla deben ser crecientes, con al menos 2 puntos por eje.</translation>
    </message>
    <message>
        <source>The grid asks for %1 cells, and the limit is %2. Reduce the number of points per axis.</source>
        <translation>La rejilla pide %1 celdas y el límite es %2. Reduzca el número de puntos por eje.</translation>
    </message>
</context>
<context>
    <name>qftbx::BoundaryUnionViewer</name>
    <message>
        <source>Boundary union</source>
        <translation>Unión de boundaries</translation>
    </message>
    <message>
        <source>Boundary plot</source>
        <translation>Gráfica de boundaries</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
</context>
<context>
    <name>qftbx::BoundaryViewer</name>
    <message>
        <source>Boundaries</source>
        <translation>Boundaries</translation>
    </message>
    <message>
        <source>Boundary plot</source>
        <translation>Gráfica de boundaries</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
</context>
<context>
    <name>qftbx::ControllerForm</name>
    <message>
        <source>Controller structure</source>
        <translation>Estructura del controlador</translation>
    </message>
    <message>
        <source>Search range of every parameter of the structure</source>
        <translation>Intervalo de búsqueda de cada parámetro de la estructura</translation>
    </message>
    <message>
        <source>Zeros:</source>
        <translation>Ceros:</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Poles:</source>
        <translation>Polos:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>One name per zero and per pole: each of them is searched for over the range given under Controller freedom.</source>
        <translation>Un nombre por cada cero y cada polo: cada uno se busca dentro del intervalo que se le dé en Libertad del controlador.</translation>
    </message>
    <message>
        <source>The coefficients by descending power. A name is searched for over the range given under Controller freedom.</source>
        <translation>Los coeficientes en potencias decrecientes. Un nombre se busca dentro del intervalo que se le dé en Libertad del controlador.</translation>
    </message>
    <message>
        <source>An expression in s. Every other name in it is searched for over its range.</source>
        <translation>Una expresión en s. Cualquier otro nombre que aparezca se busca dentro de su intervalo.</translation>
    </message>
    <message>
        <source>The structure appears here once verified.</source>
        <translation>La estructura aparece aquí en cuanto se verifique.</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>The ends of the gain range are numbers.</source>
        <translation>Los extremos del intervalo de ganancia son números.</translation>
    </message>
    <message>
        <source>This is not an expression the toolbox can read.</source>
        <translation>Esto no es una expresión que el programa sepa leer.</translation>
    </message>
    <message>
        <source>The parameters of the structure need a range: open Controller freedom.</source>
        <translation>Los parámetros de la estructura necesitan un intervalo: abra Libertad del controlador.</translation>
    </message>
    <message>
        <source>The parameters of the structure need a range.</source>
        <translation>Los parámetros de la estructura necesitan un intervalo.</translation>
    </message>
    <message>
        <source>A coefficient is not a number.</source>
        <translation>Un coeficiente no es un número.</translation>
    </message>
    <message>
        <source>Choose how the structure is written.</source>
        <translation>Elija cómo se escribe la estructura.</translation>
    </message>
</context>
<context>
    <name>qftbx::FormulaView</name>
    <message>
        <source>Copy as LaTeX</source>
        <translation>Copiar en LaTeX</translation>
    </message>
</context>
<context>
    <name>qftbx::FrequenciesForm</name>
    <message>
        <source>Design frequencies input</source>
        <translation>Frecuencias de diseño</translation>
    </message>
    <message>
        <source>A logarithmic range needs both ends greater than zero, in rad/s.</source>
        <translation>Un rango logarítmico necesita los dos extremos mayores que cero, en rad/s.</translation>
    </message>
    <message>
        <source>Enter at least one design frequency.</source>
        <translation>Introduzca al menos una frecuencia de diseño.</translation>
    </message>
    <message>
        <source>A design frequency must be a positive real, and %1 is not.</source>
        <translation>Una frecuencia de diseño debe ser un real positivo, y %1 no lo es.</translation>
    </message>
    <message>
        <source>These are not numbers separated by spaces.</source>
        <translation>Esto no son números separados por espacios.</translation>
    </message>
</context>
<context>
    <name>qftbx::FrequencyLegend</name>
    <message>
        <source>Frequencies</source>
        <translation>Frecuencias</translation>
    </message>
    <message>
        <source>filter</source>
        <translation>filtro</translation>
    </message>
    <message>
        <source>All</source>
        <translation>Todas</translation>
    </message>
    <message>
        <source>None</source>
        <translation>Ninguna</translation>
    </message>
    <message>
        <source>Shows only the frequencies whose number contains this text.</source>
        <translation>Enseña sólo las frecuencias cuyo número contiene este texto.</translation>
    </message>
    <message>
        <source>Ticks every frequency the filter is showing.</source>
        <translation>Marca todas las frecuencias que el filtro esté enseñando.</translation>
    </message>
    <message>
        <source>Unticks them.</source>
        <translation>Las desmarca.</translation>
    </message>
    <message>
        <source>Shows or hides what belongs to this frequency, in rad/s.</source>
        <translation>Enseña u oculta lo que pertenece a esta frecuencia, en rad/s.</translation>
    </message>
</context>
<context>
    <name>qftbx::LoopBoundariesViewer</name>
    <message>
        <source>Boundary union</source>
        <translation>Unión de boundaries</translation>
    </message>
    <message>
        <source>Nichols</source>
        <translation>Nichols</translation>
    </message>
    <message>
        <source>Nyquist</source>
        <translation>Nyquist</translation>
    </message>
    <message>
        <source>Boundary plot</source>
        <translation>Gráfica de boundaries</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
</context>
<context>
    <name>qftbx::LoopShapingForm</name>
    <message>
        <source>Loop-shaping input</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>Epsilon (controller parameter box width):</source>
        <translation>Épsilon (anchura de la caja de parámetros del controlador):</translation>
    </message>
    <message>
        <source>Epsilon (Nichols box diameter):</source>
        <translation>Épsilon (diámetro de la caja de Nichols):</translation>
    </message>
    <message>
        <source>The epsilon must be a positive real number.</source>
        <translation>Épsilon debe ser un número real positivo.</translation>
    </message>
    <message>
        <source>The start frequency must be a real number.</source>
        <translation>La frecuencia inicial debe ser un número real.</translation>
    </message>
    <message>
        <source>The end frequency must be a real number.</source>
        <translation>La frecuencia final debe ser un número real.</translation>
    </message>
    <message>
        <source>The point count must be a whole number of at least 1.</source>
        <translation>El número de puntos debe ser un entero de al menos 1.</translation>
    </message>
</context>
<context>
    <name>qftbx::LoopShapingViewer</name>
    <message>
        <source>Loop Shaping</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>Loop-shaping plot</source>
        <translation>Gráfica del ajuste del lazo</translation>
    </message>
    <message>
        <source>Not checked against the specifications (no templates to check over).</source>
        <translation>Sin comprobar contra las especificaciones (no hay plantillas sobre las que comprobar).</translation>
    </message>
    <message>
        <source>Satisfies every specification over the template: tightest at w = %1 rad/s, %2, %3 dB of margin.</source>
        <translation>Cumple todas las especificaciones sobre la plantilla: la más ajustada en w = %1 rad/s, %2, %3 dB de margen.</translation>
    </message>
    <message>
        <source>EXCEEDS a specification over the template: w = %1 rad/s, %2, by %3 dB.</source>
        <translation>INCUMPLE una especificación sobre la plantilla: w = %1 rad/s, %2, por %3 dB.</translation>
    </message>
    <message>
        <source>w = %1: %2 = %3 dB, bound %4 dB, excess %5 dB
</source>
        <translation>w = %1: %2 = %3 dB, cota %4 dB, exceso %5 dB
</translation>
    </message>
    <message>
        <source>tracking</source>
        <translation>seguimiento</translation>
    </message>
    <message>
        <source>stability</source>
        <translation>estabilidad</translation>
    </message>
    <message>
        <source>sensor noise</source>
        <translation>ruido del sensor</translation>
    </message>
    <message>
        <source>output disturbance</source>
        <translation>perturbación a la salida</translation>
    </message>
    <message>
        <source>input disturbance</source>
        <translation>perturbación a la entrada</translation>
    </message>
    <message>
        <source>control effort</source>
        <translation>esfuerzo de control</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
    <message>
        <source>No specification was active at any design frequency.</source>
        <translation>No había ninguna especificación activa en ninguna frecuencia de diseño.</translation>
    </message>
    <message>
        <source>Satisfies every specification over the template, by %1 dB.</source>
        <translation>Cumple todas las especificaciones sobre la plantilla, por %1 dB.</translation>
    </message>
    <message>
        <source>EXCEEDS a specification over the template by %1 dB.</source>
        <translation>INCUMPLE una especificación sobre la plantilla por %1 dB.</translation>
    </message>
    <message>
        <source>all</source>
        <translation>todas</translation>
    </message>
</context>
<context>
    <name>qftbx::MainWindow</name>
    <message>
        <source>&amp;Tools</source>
        <translation>&amp;Herramientas</translation>
    </message>
    <message>
        <source>Benchmark &amp;planner...</source>
        <translation>&amp;Planificador de pruebas...</translation>
    </message>
    <message>
        <source>QFT: Quantitative feedback theory</source>
        <translation>QFT: teoría de la realimentación cuantitativa</translation>
    </message>
    <message>
        <source>&amp;Language</source>
        <translation>&amp;Idioma</translation>
    </message>
    <message>
        <source>QFT Files (*.qft)</source>
        <translation>Ficheros QFT (*.qft)</translation>
    </message>
    <message>
        <source>Template computation</source>
        <translation>Cálculo de templates</translation>
    </message>
    <message>
        <source>Specifications input</source>
        <translation>Especificaciones</translation>
    </message>
    <message>
        <source>Boundary computation</source>
        <translation>Cálculo de boundaries</translation>
    </message>
    <message>
        <source>Loop Shaping</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>Save file</source>
        <translation>Guardar fichero</translation>
    </message>
    <message>
        <source>Open project</source>
        <translation>Abrir proyecto</translation>
    </message>
    <message>
        <source>To show the Bode diagram, first enter a valid plant and a set of design frequencies</source>
        <translation>Para mostrar el diagrama de Bode, introduzca antes una planta válida y un conjunto de frecuencias de diseño</translation>
    </message>
    <message>
        <source>QFT</source>
        <translation>QFT</translation>
    </message>
    <message>
        <source>To show the loop diagram, first compute the boundaries and enter the controller structure.</source>
        <translation>Para mostrar el diagrama del lazo, calcule antes los boundaries e introduzca la estructura del controlador.</translation>
    </message>
    <message>
        <source>Language</source>
        <translation>Idioma</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>Ay&amp;uda</translation>
    </message>
    <message>
        <source>&amp;About QFTbx...</source>
        <translation>&amp;Acerca de QFTbx...</translation>
    </message>
    <message>
        <source>About &amp;Qt...</source>
        <translation>Acerca de &amp;Qt...</translation>
    </message>
    <message>
        <source>Plant</source>
        <translation>Planta</translation>
    </message>
    <message>
        <source>Bode</source>
        <translation>Bode</translation>
    </message>
    <message>
        <source>Specifications</source>
        <translation>Especificaciones</translation>
    </message>
    <message>
        <source>Design frequencies</source>
        <translation>Frecuencias de diseño</translation>
    </message>
    <message>
        <source>Templates</source>
        <translation>Templates</translation>
    </message>
    <message>
        <source>Boundaries</source>
        <translation>Boundaries</translation>
    </message>
    <message>
        <source>Per frequency</source>
        <translation>Por frecuencia</translation>
    </message>
    <message>
        <source>Union</source>
        <translation>Unión</translation>
    </message>
    <message>
        <source>Controller structure</source>
        <translation>Estructura del controlador</translation>
    </message>
    <message>
        <source>Loop shaping</source>
        <translation>Ajuste del lazo</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Lazo</translation>
    </message>
    <message>
        <source>Bode diagram</source>
        <translation>Diagrama de Bode</translation>
    </message>
    <message>
        <source>Appearance</source>
        <translation>Aspecto</translation>
    </message>
    <message>
        <source>&amp;Appearance</source>
        <translation>&amp;Aspecto</translation>
    </message>
    <message>
        <source>Templates: sweeping...</source>
        <translation>Templates: barriendo…</translation>
    </message>
    <message>
        <source>Boundaries: computing...</source>
        <translation>Boundaries: calculando…</translation>
    </message>
    <message>
        <source>Loop: searching...</source>
        <translation>Lazo: buscando…</translation>
    </message>
    <message>
        <source>Another phase is computing. Wait for it or cancel it.</source>
        <translation>Hay otra fase calculando. Espere a que termine o cancélela.</translation>
    </message>
    <message>
        <source>A computation is already running.</source>
        <translation>Ya hay un cálculo en marcha.</translation>
    </message>
</context>
<context>
    <name>qftbx::PhaseCard</name>
    <message>
        <source>Data</source>
        <translation>Datos</translation>
    </message>
    <message>
        <source>Show or hide what this phase was asked for</source>
        <translation>Mostrar u ocultar lo que se le pidió a esta fase</translation>
    </message>
    <message>
        <source>Narrower</source>
        <translation>Más estrecho</translation>
    </message>
    <message>
        <source>Wider</source>
        <translation>Más ancho</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>Give up on the computation of this phase</source>
        <translation>Abandonar el cálculo de esta fase</translation>
    </message>
    <message>
        <source>Close this phase. Its button at the top of the window opens it again, with everything it holds.</source>
        <translation>Cerrar esta fase. Su botón de arriba vuelve a abrirla, con todo lo que tenga.</translation>
    </message>
</context>
<context>
    <name>qftbx::PlantForm</name>
    <message>
        <source>Plant</source>
        <translation>Planta</translation>
    </message>
    <message>
        <source>Uncertainty of the plant</source>
        <translation>Incertidumbre de la planta</translation>
    </message>
    <message>
        <source>Choose how the plant is written.</source>
        <translation>Elija cómo se escribe la planta.</translation>
    </message>
    <message>
        <source>Zeros:</source>
        <translation>Ceros:</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Poles:</source>
        <translation>Polos:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>One zero and one pole per value, separated by spaces: the factors are (s + a). A name instead of a number makes that root uncertain.</source>
        <translation>Un cero y un polo por valor, separados por espacios: los factores son (s + a). Un nombre en vez de un número hace esa raíz incierta.</translation>
    </message>
    <message>
        <source>One time constant per value, separated by spaces: the factors are (1 + s/T). A name instead of a number makes that constant uncertain.</source>
        <translation>Una constante de tiempo por valor, separadas por espacios: los factores son (1 + s/T). Un nombre en vez de un número hace esa constante incierta.</translation>
    </message>
    <message>
        <source>The coefficients by descending power, separated by spaces. A name instead of a number makes that coefficient uncertain.</source>
        <translation>Los coeficientes en potencias decrecientes, separados por espacios. Un nombre en vez de un número hace ese coeficiente incierto.</translation>
    </message>
    <message>
        <source>An expression in s. Every other name in it is an uncertain parameter.</source>
        <translation>Una expresión en s. Cualquier otro nombre que aparezca es un parámetro incierto.</translation>
    </message>
    <message>
        <source>The formula appears here once verified.</source>
        <translation>La fórmula aparece aquí en cuanto se verifique.</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>The plant needs a name.</source>
        <translation>La planta necesita un nombre.</translation>
    </message>
    <message>
        <source>%1 is a value; its range is given under Uncertainty.</source>
        <translation>%1 es un valor; su intervalo se da en Incertidumbre.</translation>
    </message>
    <message>
        <source>%1 is not a number.</source>
        <translation>%1 no es un número.</translation>
    </message>
    <message>
        <source>The gain</source>
        <translation>La ganancia</translation>
    </message>
    <message>
        <source>The delay</source>
        <translation>El retardo</translation>
    </message>
    <message>
        <source>This is not an expression the toolbox can read.</source>
        <translation>Esto no es una expresión que el programa sepa leer.</translation>
    </message>
    <message>
        <source>The coefficients that were given a name need a range: open Uncertainty.</source>
        <translation>Los coeficientes a los que se dio un nombre necesitan un intervalo: abra Incertidumbre.</translation>
    </message>
    <message>
        <source>The coefficients that were given a name need a range.</source>
        <translation>Los coeficientes a los que se dio un nombre necesitan un intervalo.</translation>
    </message>
    <message>
        <source>The gain is not a number.</source>
        <translation>La ganancia no es un número.</translation>
    </message>
    <message>
        <source>The delay is not a number.</source>
        <translation>El retardo no es un número.</translation>
    </message>
    <message>
        <source>A coefficient is not a number.</source>
        <translation>Un coeficiente no es un número.</translation>
    </message>
</context>
<context>
    <name>qftbx::SpecificationsForm</name>
    <message>
        <source>The magnitude must be a finite number, and positive in linear units.</source>
        <translation>La magnitud debe ser un número finito, y positivo en unidades lineales.</translation>
    </message>
    <message>
        <source>The design frequencies must be entered before the specifications.</source>
        <translation>Hay que introducir las frecuencias de diseño antes que las especificaciones.</translation>
    </message>
    <message>
        <source>Specifications</source>
        <translation>Especificaciones</translation>
    </message>
    <message>
        <source>Tracking, lower bound</source>
        <translation>Seguimiento, cota inferior</translation>
    </message>
    <message>
        <source>Tracking, upper bound</source>
        <translation>Seguimiento, cota superior</translation>
    </message>
    <message>
        <source>Stability</source>
        <translation>Estabilidad</translation>
    </message>
    <message>
        <source>Sensor noise</source>
        <translation>Ruido del sensor</translation>
    </message>
    <message>
        <source>Output disturbance</source>
        <translation>Perturbación a la salida</translation>
    </message>
    <message>
        <source>Input disturbance</source>
        <translation>Perturbación a la entrada</translation>
    </message>
    <message>
        <source>Control effort</source>
        <translation>Esfuerzo de control</translation>
    </message>
    <message>
        <source>Specification</source>
        <translation>Especificación</translation>
    </message>
    <message>
        <source>Band (rad/s)</source>
        <translation>Banda (rad/s)</translation>
    </message>
    <message>
        <source>Bound</source>
        <translation>Cota</translation>
    </message>
    <message>
        <source>Zeros:</source>
        <translation>Ceros:</translation>
    </message>
    <message>
        <source>Numerator:</source>
        <translation>Numerador:</translation>
    </message>
    <message>
        <source>Poles:</source>
        <translation>Polos:</translation>
    </message>
    <message>
        <source>Denominator:</source>
        <translation>Denominador:</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation>Verificar</translation>
    </message>
    <message>
        <source>Update</source>
        <translation>Actualizar</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Añadir</translation>
    </message>
    <message>
        <source>%1 to %2</source>
        <translation>%1 a %2</translation>
    </message>
    <message>
        <source>The band needs 0 &lt;= start &lt;= end.</source>
        <translation>La banda necesita 0 &lt;= inicio &lt;= fin.</translation>
    </message>
    <message>
        <source>The bound is a magnitude.</source>
        <translation>La cota es una magnitud.</translation>
    </message>
    <message>
        <source>The gain is not a number.</source>
        <translation>La ganancia no es un número.</translation>
    </message>
    <message>
        <source>The delay is not a number.</source>
        <translation>El retardo no es un número.</translation>
    </message>
    <message>
        <source>A free-form bound needs both expressions.</source>
        <translation>Una cota en formato libre necesita las dos expresiones.</translation>
    </message>
    <message>
        <source>This is not an expression the toolbox can read.</source>
        <translation>Esto no es una expresión que el programa sepa leer.</translation>
    </message>
    <message>
        <source>The bound needs a denominator.</source>
        <translation>La cota necesita un denominador.</translation>
    </message>
    <message>
        <source>A coefficient is not a number.</source>
        <translation>Un coeficiente no es un número.</translation>
    </message>
    <message>
        <source>Added: %1.</source>
        <translation>Añadida: %1.</translation>
    </message>
    <message>
        <source>Choose a specification in the list first.</source>
        <translation>Elija antes una especificación de la lista.</translation>
    </message>
    <message numerus="yes">
        <source> (%n out)</source>
        <translation>
            <numerusform> (%n fuera)</numerusform>
            <numerusform> (%n fuera)</numerusform>
        </translation>
    </message>
    <message>
        <source>A specification applies at least at one frequency.</source>
        <translation>Una especificación se aplica al menos en una frecuencia.</translation>
    </message>
    <message>
        <source>The loop shaping bounds the spread of the closed loop over the plant family against the difference between the two tracking bounds: the prefilter F shifts the band and cannot narrow it, and it is designed afterwards.</source>
        <translation>El ajuste del lazo acota la dispersión del lazo cerrado sobre la familia de plantas contra la diferencia entre las dos cotas de seguimiento: el prefiltro F desplaza la banda pero no la estrecha, y se diseña después.</translation>
    </message>
</context>
<context>
    <name>qftbx::TemplateViewer</name>
    <message>
        <source>Templates</source>
        <translation>Templates</translation>
    </message>
    <message>
        <source>Template plot</source>
        <translation>Gráfica de templates</translation>
    </message>
    <message>
        <source>Hide
templates</source>
        <translation>Ocultar
templates</translation>
    </message>
    <message>
        <source>Show
templates</source>
        <translation>Mostrar
templates</translation>
    </message>
    <message>
        <source>Hide
contour</source>
        <translation>Ocultar
contorno</translation>
    </message>
    <message>
        <source>Show
contour</source>
        <translation>Mostrar
contorno</translation>
    </message>
    <message>
        <source>no contour: whole template shown</source>
        <translation>sin contorno: se muestra la plantilla entera</translation>
    </message>
    <message>
        <source>No contour closed at this epsilon, so the whole template stands in for it here. A larger epsilon, or a denser sweep, closes it.</source>
        <translation>Ningún contorno cerró con este épsilon, así que aquí la plantilla entera hace de contorno. Un épsilon mayor, o un barrido más denso, lo cierra.</translation>
    </message>
    <message>
        <source>needs %1 (gap %2%)</source>
        <translation>pide %1 (hueco %2 %)</translation>
    </message>
    <message>
        <source>The least epsilon at which this template&apos;s contour closes is %1 (it is connected from %2); the largest gap between its points is %3% of its size. Above a few per cent the sweep is coarse: more points per parameter, not a larger epsilon.</source>
        <translation>El menor épsilon con el que cierra el contorno de esta plantilla es %1 (está conectada desde %2); el hueco mayor entre sus puntos es el %3 % de su tamaño. Por encima de unos pocos por ciento el barrido es escaso: más puntos por parámetro, no un épsilon mayor.</translation>
    </message>
    <message>
        <source>phase (degrees)</source>
        <translation>fase (grados)</translation>
    </message>
    <message>
        <source>magnitude (dB)</source>
        <translation>magnitud (dB)</translation>
    </message>
    <message>
        <source>open contour</source>
        <translation>contorno abierto</translation>
    </message>
    <message>
        <source>The epsilon-hull walk did not close at this epsilon and the relaxed walk stood in for it: what is drawn covers the cloud but is not the closed hull, so it ends where the walk ended. Propose gives the epsilon that closes it.</source>
        <translation>El recorrido de la envoltura-épsilon no cerró con este épsilon y ha entrado en su lugar el recorrido relajado: lo que se dibuja cubre la nube pero no es la envoltura cerrada, así que termina donde terminó el recorrido. Proponer da el épsilon que la cierra.</translation>
    </message>
    <message>
        <source>The epsilon of this frequency: the diameter of the hull the contour of this template is walked with. Recompute walks the contours again with it.</source>
        <translation>El épsilon de esta frecuencia: el diámetro de la envoltura con la que se recorre el contorno de esta plantilla. Recalcular recorre otra vez los contornos con él.</translation>
    </message>
</context>
<context>
    <name>qftbx::TemplatesForm</name>
    <message>
        <source>Template input</source>
        <translation>Templates</translation>
    </message>
    <message>
        <source>LinSpace</source>
        <translation>LinSpace</translation>
    </message>
    <message>
        <source>LogSpace</source>
        <translation>LogSpace</translation>
    </message>
    <message>
        <source>Manual</source>
        <translation>Manual</translation>
    </message>
    <message>
        <source>No epsilon value was entered.</source>
        <translation>No se ha introducido ningún valor de épsilon.</translation>
    </message>
    <message>
        <source>Template computation</source>
        <translation>Cálculo de templates</translation>
    </message>
    <message>
        <source>Invalid epsilon expression.</source>
        <translation>Expresión de épsilon no válida.</translation>
    </message>
    <message>
        <source>Every epsilon must be a positive finite number.</source>
        <translation>Cada épsilon debe ser un número finito positivo.</translation>
    </message>
    <message>
        <source>Select logspace or linspace in the general section.</source>
        <translation>Seleccione logspace o linspace en la sección general.</translation>
    </message>
    <message>
        <source>The values entered for parameter &quot;%1&quot; are invalid.</source>
        <translation>Los valores introducidos para el parámetro &quot;%1&quot; no son válidos.</translation>
    </message>
    <message>
        <source>The values entered for parameter &quot;%1&quot; are invalid: %2.</source>
        <translation>Los valores introducidos para el parámetro &quot;%1&quot; no son válidos: %2.</translation>
    </message>
    <message>
        <source>The general point count must be a whole number between 1 and %1.</source>
        <translation>El número de puntos general debe ser un entero entre 1 y %1.</translation>
    </message>
    <message>
        <source>Invalid grid expressions.</source>
        <translation>Expresiones de la rejilla no válidas.</translation>
    </message>
    <message>
        <source>The parameter name(s) %1 appear more than once: the first grid entered is used for every occurrence.</source>
        <translation>Los nombres de parámetro %1 aparecen más de una vez: la primera rejilla introducida se usa en todas las apariciones.</translation>
    </message>
    <message>
        <source>its point count must be a whole number between 1 and %1</source>
        <translation>su número de puntos debe ser un entero entre 1 y %1</translation>
    </message>
    <message>
        <source>one of its grid values is not a finite number</source>
        <translation>uno de los valores de su rejilla no es un número finito</translation>
    </message>
    <message>
        <source>frequency %1: %2 (connected from %3, gap %4%)</source>
        <translation>frecuencia %1: %2 (conectada desde %3, hueco %4 %)</translation>
    </message>
    <message>
        <source>frequency %1: %2 (connected, but no epsilon up to the diameter closes the walk; gap %3%)</source>
        <translation>frecuencia %1: %2 (conectada, pero ningún épsilon hasta el diámetro cierra el recorrido; hueco %3 %)</translation>
    </message>
    <message>
        <source>The least epsilon at which the contour of each template closes, over the grids as entered; below the connecting value the template splits. The gap is the largest distance between neighbouring points of the template as a share of its size: above a few per cent the sweep is coarse and asks for more points, not a larger epsilon.
%1</source>
        <translation>El menor épsilon con el que cierra el contorno de cada plantilla, sobre las mallas tal como están; por debajo del valor de conexión la plantilla se parte. El hueco es la mayor distancia entre puntos vecinos de la plantilla como fracción de su tamaño: por encima de unos pocos por ciento el barrido es escaso y pide más puntos, no un épsilon mayor.
%1</translation>
    </message>
    <message>
        <source>The decibels per degree must be a positive number.</source>
        <translation>Los decibelios por grado deben ser un número positivo.</translation>
    </message>
    <message>
        <source>The plant must be entered before the templates.</source>
        <translation>Hay que introducir la planta antes que los templates.</translation>
    </message>
</context>
<context>
    <name>qftbx::Theme</name>
    <message>
        <source>Light</source>
        <translation>Claro</translation>
    </message>
    <message>
        <source>Dark</source>
        <translation>Oscuro</translation>
    </message>
    <message>
        <source>System theme</source>
        <translation>Tema del sistema</translation>
    </message>
</context>
<context>
    <name>qftbx::UncertaintyPanel</name>
    <message>
        <source>Parameter</source>
        <translation>Parámetro</translation>
    </message>
    <message>
        <source>Minimum</source>
        <translation>Mínimo</translation>
    </message>
    <message>
        <source>Nominal</source>
        <translation>Nominal</translation>
    </message>
    <message>
        <source>Maximum</source>
        <translation>Máximo</translation>
    </message>
    <message>
        <source>The range of &quot;%1&quot; is not a pair of numbers with the nominal value between them.</source>
        <translation>El intervalo de &quot;%1&quot; no es un par de números con el valor nominal entre ellos.</translation>
    </message>
    <message>
        <source>A coefficient that is not uncertain is not a number either.</source>
        <translation>Un coeficiente que no es incierto tampoco es un número.</translation>
    </message>
    <message>
        <source>The smallest value &quot;%1&quot; takes. The templates are swept over the whole interval.</source>
        <translation>El menor valor que toma &quot;%1&quot;. Las plantillas se barren sobre todo el intervalo.</translation>
    </message>
    <message>
        <source>The largest value &quot;%1&quot; takes.</source>
        <translation>El mayor valor que toma &quot;%1&quot;.</translation>
    </message>
    <message>
        <source>The value &quot;%1&quot; has in the NOMINAL plant, the one the loop is shaped on. It has to lie inside the interval.</source>
        <translation>El valor que &quot;%1&quot; tiene en la planta NOMINAL, sobre la que se ajusta el lazo. Tiene que estar dentro del intervalo.</translation>
    </message>
</context>
</TS>
