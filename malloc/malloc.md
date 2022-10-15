# TP: malloc

## Regiones
En un comienzo, la implementacion se realizo mediantes regiones en un unico bloque de tamaño fijo. Estas regiones tenian un header, que consistia en:
- Tamaño de la region
- Si estaba, o no, liberada
- Puntero a la proxima region

Posteriormente, se encontro que agregando un puntero a la region previa,creando una lista doblemente enlazada, se podia mejorar y hacer mas facil la operacion de *coalesce*.

Luego, tambien, se agrego un puntero al header del bloque en el que la region vive y un *numero magico*. 
- El puntero al bloque se utiliza para hacer la operacion de devolucion de memoria al S.O. mas eficiente. 
- El *numero magico* se utilizo para saber si un puntero pasado a las funciones de `free` y `realloc` es valido. 

## Bloques
La implementacion de los bloques se realizo mediante un header, que no esta incluido dentro del tamaño del **bloque** (lease, la memoria disponible para el usuario), el cual contiene la siguiente información:
- Tamaño del bloque
- Puntero al siguiente bloque
- Puntero al bloque previo

La estructura de lista doblemente enlazada ayudo a que la liberacion(devolver al S.O.) de un bloque cualquiera en la lista sea mas eficiente.

Este header se encuentra ubicado previo al **bloque**.

## Operaciones

### Realloc

Para hacer el realloc se tiene en cuenta varios diferentes casos que pueden ocurrir durante el ciclo de vida del programa:

- **Si el nuevo tamaño es menor al tamaño actual de la region**: Se intenta *splittear* la region para poder generar una nueva region libre. Se devuelve al usuario el mismo puntero, pero con el tamaño cambiado (de ser necesario ).
- **Si el nuevo tamaño es igual** se devuelve el mismo puntero, sin realizar cambios. 
- **Si el nuevo tamaño es mayor al tamaño actual de la region**
    - **Si la region siguiente esta libre y la suma de los tamaños es suficiente** Se juntan ambas regiones y se devuelve el mismo puntero, con el tamaño cambiado en el *header*. De ser necesario y posible, se realiza un *split* para no generar fragmentacion interna.
    - **Si la region previa esta libre y la suma de los tamaños es suficiente** Se juntan las regiones, moviendo la informacion del usuario al lugar correcto. Se realiza un *split* de ser necesario y se actualiza el tamaño en el *header*. El puntero retornado al usuario no sera el mismo.
    - **Si la region siguiente y previa esta libre, y la suma de los tamaños es suficiente** Se juntan las tres regiones, moviendo la informacion del usuario al lugar correcto. Se realiza un *split* de ser necesario y se actualiza el tamaño en el *header*. El puntero retornado al usuario no sera el mismo.
    - **En cualquier otro caso** Se pide una nueva region, llamando a `malloc`, copiando la informacion del usuario a la nueva region devuelta. Se libera la anterior region(utilizando `free`)
### Validez del puntero 

Para decidir si el puntero pasado a las funciones `free` o `realloc` se considera valido, la direccion del mismo debe estar dentro de las direcciones de los bloques. Tambien tiene que ocurrir que el *magic number* debe concidir para un puntero dado. 

### Coalesce

Cada vez se libera una region, se intenta realizar *coalesce* con las regiones adyacentes, de "derecha a izquierda". Dado que esta operacion se realiza en cada liberacion de region, se puede asegurar que todas las otras regiones no involucradas en la operacion estan lo mas "juntadas" posibles. De esta forma, solo se tiene que realizar el *coalesce* con la region liberada. 

### Split

Cuando se realiza un *split* de una region, se asegura que la potencial nueva region, tenga al menos tamaño suficiente para el header y un tamaño de region minimo (64 bytes <-- eleccion arbitraria). En caso de no ser asi, no se realiza el split y el usuario tendra regiones que podria ser mas grande que lo pedido.

## Decisiones de diseño

**Tamaño minimo de una region**: Se decidio por 64 bytes.

**Magic number**: Numero aleatorio.

### **Estadisticas**

Se definen estadisticas para poder probar el funcionamiento de la libreria. La actualizacion y utilizacion de las estadisticas dentro de la libreria es decidible a tiempo de compilacion. Si `USE_STATS` esta definido a la hora compilar , la libreria actualizara las estadisticas a medida que se vaya utilizando; si no lo esta, no se compilara el codigo referente a las estadisticas. 

Estas estadisticas refieren tanto a cantidad de llamadas a las funciones, cantidad de memoria pedida y liberada, cantidad de regiones y bloques, cuantas veces realizo la operacion de coalesce y cuantas veces realizo la operacion de split. 