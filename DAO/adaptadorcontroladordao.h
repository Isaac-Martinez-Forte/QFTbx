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


      LtiSystem * getControlador ();

      void setControlador (LtiSystem * controlador);

  private:
      LtiSystem * controlador = NULL;
};

#endif // ADAPTADORCONTROLADORDAO_H
