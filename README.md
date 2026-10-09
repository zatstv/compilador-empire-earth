# Compilador Empire Earth

![Empire Earth](img/portada.jpg)

Proyecto final de Compiladores (IS-581). Es un compilador para un lenguaje que inventé basado en el juego Empire Earth. Las variables son "unidades" que se reclutan y cada tipo de dato es algo del juego.

Está hecho en C con Flex y Bison.

## El lenguaje

Todo programa empieza con `fundar_ciudad` y termina con `victoria`.

Para declarar se usa `reclutar`, y para asignar se usa `<-`:

```
fundar_ciudad
    reclutar recurso comida, madera, oro;
    reclutar epoca era <- 1;
    reclutar moral animo;
    reclutar heroe lider <- "Alejandro";
    reclutar aliado vecino;

    comida <- 200;
    madera <- 150;
    oro    <- comida;
    era    <- 5;
    animo  <- 3;
    vecino <- paz;
victoria
```

### Estrategias (funciones)

Una estrategia es una función. Se crea con `estrategia`, los parámetros llevan el tipo antes del nombre, y después de `:` va el tipo de lo que devuelve. Para devolver un valor se usa `tributo`. Si no devuelve nada el tipo es `nada`.

```
estrategia juntar(recurso a, recurso b): recurso {
    tributo a + b;
}

estrategia firmar_paz(heroe nombre): nada {
    lider <- nombre;
    vecino <- paz;
}

oro <- juntar(comida, madera) * 2;
firmar_paz("Carlomagno");
```

Con los números se puede sumar, restar, multiplicar y dividir (`+ - * /`).

### Tipos

| Tipo | Qué guarda | Valor inicial |
|------|------------|---------------|
| recurso | números enteros (0 o más) | 0 |
| epoca | número entero del 1 al 14, como las épocas del juego | 1 |
| moral | número entero del 0 al 5, como los puntos de moral de las unidades | 0 |
| heroe | texto entre comillas | "" |
| aliado | `paz` o `guerra` | guerra |

La `epoca` va del 1 al 14 porque en el juego hay 14 épocas, desde la Prehistoria hasta la Era Nano.

| | |
|---|---|
| ![Edad del Cobre](img/edad_cobre.jpg) | ![Edad Media](img/edad_media.jpg) |
| Edad del Cobre | Edad Media |
| ![Segunda Guerra Mundial](img/segunda_guerra.jpg) | ![Era Nano](img/era_nano.jpg) |
| Segunda Guerra Mundial | Era Nano |

### Reglas

- Cada instrucción termina en `;`.
- No se puede usar una unidad sin reclutarla antes.
- No se puede reclutar dos veces la misma unidad.
- El valor tiene que ser del tipo de la unidad.
- Una `epoca` solo puede valer de 1 a 14.
- Una `moral` solo puede valer de 0 a 5.
- Los nombres no pueden empezar con número ni llevar tildes o ñ.
- Los comentarios empiezan con `#`.
- Una estrategia se tiene que crear antes de usarla y no puede ir dentro de otra.
- Hay que pasarle la misma cantidad de parámetros que pide y del tipo correcto.
- Si la estrategia no es de tipo `nada` tiene que entregar `tributo`.
- `tributo` solo se puede usar dentro de una estrategia.
- Las unidades que se reclutan dentro de una estrategia solo existen ahí.
- Un recurso no puede quedar negativo y no se puede dividir entre 0.

Si algo está mal el compilador muestra `DERROTA en la linea N:` y qué fue lo que pasó. Si todo sale bien muestra `VICTORIA`.

## Qué hace el compilador

1. Análisis léxico y sintáctico (`lexer.l` y `parser.y`)
2. Arma el árbol de sintaxis (AST) y lo muestra como tabla (`ast.c`)
3. Análisis semántico: revisa que las unidades y estrategias existan y que los tipos cuadren (`semantic.c`)
4. Ejecuta el programa (`runtime.c`)
5. Muestra la tabla de símbolos con las unidades, estrategias y parámetros, en qué ámbito están y el valor final (`symbol_table.c`)

## Cómo correrlo

Se necesita `gcc`, `flex`, `bison` y `make`.

```
make
./compiler source.ee
```

Con `-t` también muestra los tokens:

```
./compiler -t source.ee
```

Para borrar lo compilado:

```
make clean
```

## Capturas del juego

| | |
|---|---|
| ![](img/menu.jpg) | ![](img/ejercito.jpg) |
| ![](img/heroe.jpg) | ![](img/batalla_naval.jpg) |
| ![](img/primera_guerra.jpg) | ![](img/ciudad_nano.jpg) |
| ![](img/civilizacion.jpg) | ![](img/editor.jpg) |
