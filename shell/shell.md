# Lab: shell

### Búsqueda en $PATH
**Q**: ¿Cuales son las diferencias entre la *syscall* `execve(2)` y la familia de *wrappers* proporcionados por la libreria estandar de C *(libc)* `exec(3)`

**A**: Los wrappers son funciones que se apoyan sobre la syscall, siendo esta llamada despues de que el wrapper realize su funcion. La funcion del wrapper es proveer al programador funcionalidades frecuentemente usadas antes de llamar a exec. Estas funcionalidades varian segun el wrapper que se llame, y pueden ir desde utilizar el ENV del proceso actual, hasta facilitar la busqueda del binario a ejecutar, utilizando la variable de entorno PATH para hacerlo. La syscall provee mas granularidad, para casos extraordinarios, a la hora de ejecutar otro binario. 

**Q**: ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la *shell* en ese caso?

**A**: La llamada a cualquier wrapper puede fallar por diferentes motivos. Desde que no se encontro el binario hasta que el kernel prohibio la ejecucion por diferentes motivos. En este caso la *shell* debera cerrar el proceso hijo que creo para ejecutar el programa e informar al usuario el motivo por el cual no pudo ejecutar lo pedido (el error debe ser proveniente desde la shell, no del programa a ejecutar)

---

### Comandos built-in

**Q**: ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser *built-in*? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como *built-in*?

**A**: `pwd` se podria implementar como un programa separado de la shell, pues al realizar el `fork` + `exec`, el nuevo programa hereda el directorio actual del proceso padre. Desde ahi lo unico que haria falta es 'imprimir' el directorio actual. La razon por la que se realizan como *built-in* puede ser velocidad (se evita el *overhead* de crear un proceso, con todo lo que eso implica) y facilidad a la hora de implementarlo (sabiendo que ciertos datos se tienen como datos en la *shell*) 

---

### Variables de entorno temporarias

**Q**: En algunos de los wrappers de la familia de funciones de `exec`(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv`(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de `exec`(3).

* ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
* Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.


**A**: El comportamiento de utilizar la familia de *wrapper* `exec[v|l]pe` es de reemplazar el *environment* del proceso actual completamente por las variables pasadas en el tercer argumento (`char *const envp[]`). Puede ser que el comportamiento del programa ejecutado varie o no, dependiendo de si usa las variables de entorno. 

Para imitar el comportamiento de la familia de *wrappers* sin el tercer argumento, utilizando `setenv`, se debe concatenar el ambiente actual, con el nuevo ambiente, para luego pasar eso como tercer argumento. El ambiente del programa actual se encuentra en la variable globar `char **environ`.

```C
...
char **new_environ = ...;
extern char **environ;
char **full_environ = concat(environ, new_environ);
execvpe(argv[0], argv, full_environ);
...

```

---

### Procesos en segundo plano

**Q**: Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

**A**: Para la creacion de procesos en segundo plano, basta con realizar un `fork` + `exec` desde la *shell*. La *shell* se puede 'olvidar' de este proceso, dando al usuario la posibilidad de seguir utilizandola. La *shell* se encarga de que los procesos terminados en segundo plano sean 'matados' debidamente, asi evitando la acumulacion de procesos zombies.   

---

### Flujo estándar

**Q**: Investigar el significado de `2>&1`, explicar cómo funciona su forma general y mostrar qué sucede con la salida de `cat out.txt` en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?

**A**: `2>&1` va a redirigir la `STDERR` a donde este apuntando `STDOUT` al momento en el que se encontro la redireccion. Es decir, si `STDOUT` estaba redirigido antes de parsear esta redireccion, `STDERR` escribira en el mismo lugar que `STDOUT`; en cambio, si la redireccion esta antes de cualquier redireccion de `STDOUT`, `STDERR` sera redirigido a donde sea que apunte `STDOUT` normalmente (por lo general, la terminal).

La forma general `n>&m` funcionara de la misma manera, redirigiendo el *file descriptor* `n` a donde sea que apunte `m`. 

```bash
> ls -C /home /noexiste >out.txt 2>&1
> cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
tomas
```

```bash
> ls -C /home /noexiste 2>out.txt 1>&2
> cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
tomas
```

```bash
> ls -C /home /noexiste 2>&1 >out.txt 
ls: cannot access '/noexiste': No such file or directory
> cat out.txt
/home:
tomas
```


---

### Tuberías simples (pipes)

**Q**: Investigar qué ocurre con el *exit code* reportado por la *shell* si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab.

**A**: En bash, por defecto, se ejecutaran todos los comandos de la pipe, sin importar el resultado del comando que depende. Esto es, si algun proceso en el medio termina con un codigo distinto a 0, el siguiente proceso se ejecutara de todas maneras, utilizando un `stdout` 'roto'.  El *exit status* que reportara es el *exit status* del ultimo proceso. *Bash* guarda un array con los *exit status* de los diferentes procesos que corrieron en el pipe.

Este comportamiento se puede cambiar habilitando la opcion `pipefail`, que hace que se guarde el *exit code* del comando mas a la derecha que termino con un estado diferente de 0 (o 0 en caso de que todos hayan terminado correctamente).

En la implementacion del lab, una pipe reportara *exit code* de 0 siempre, a menos que fallen ciertas *SYS CALLS* (entiendase `fork` y `pipe`) a la hora de 'manejar' la rama derecha derecha e izquierda de la pipe.
Esto se debe a que la funcion `run_cmd` solo esta interesada en el *exit status* del '*pipe manager*'.

*Bash*
```bash
> true | false | true
> echo $?
0
...
> true | false | false
> echo $?
1
...
> true | false | true
> echo "${PIPESTATUS[0]} ${PIPESTATUS[1]} ${PIPESTATUS[2]}"
0 1 0
```

```bash
> set -o pipefail
> true | false | true
> echo $?
1
> true | false | cat - ./noexiste ; echo $? 
cat: ./noexiste: No such file or directory
1
```

*Lab-shell*
```bash
> true | false | true
> echo $?
0

> true | false | false
> echo $?
0
```

---

### Pseudo-variables

**Q**: Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

**A**:
* `!` : Devuelve el ID del ultimo proceso ejecutado en background
```bash
> sleep 10 &
223
> echo $!
223
``` 

* `@`;`*` : Expande a la lista completa de variables posicionales. `@` expande a la lista y siempre se separa con espacios; `*` expande a la lista separa por el primer caracter de la variable IFS
```bash
> function variables_posicionales(){
    IFS="-"
    echo "Variables posicionales @: ${@}"
    echo "Variables posicionales *: ${*}"
}
> variables_posicionales uno dos tres
Variables posicionales @: uno dos tres
Variables posicionales *: uno-dos-tres
```

* `-`: Expande a los flags que tiene la shell actual.
```bash
> function shell_variables(){
    echo "Variables antes de set: ${-}"
    set -x
    echo "Variables despues de set: ${-}"
}
> shell_variables
Variables antes de set: himBHs
+ echo 'Variables despues de set: himxBHs'
Variables despues de set: himxBHs
```

---


