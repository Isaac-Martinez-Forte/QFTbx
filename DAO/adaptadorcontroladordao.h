#ifndef ADAPTADORCONTROLADORDAO_H
#define ADAPTADORCONTROLADORDAO_H

#include "controladordao.h"

class AdaptadorControladorDAO : public ControladorDAO
{
public:

    /**
      * @fn AdaptadorControladorDAO
      * @brief Constructor de la clase.
     */

      AdaptadorControladorDAO();
      ~AdaptadorControladorDAO();


      Sistema * getControlador ();

      void setControlador (Sistema * controlador);

  private:
      Sistema * controlador = NULL;
};

#endif // ADAPTADORCONTROLADORDAO_H
