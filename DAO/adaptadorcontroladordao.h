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


      std::shared_ptr<Sistema> getControlador();

      void setControlador (std::shared_ptr<Sistema> controlador);

  private:
      std::shared_ptr<Sistema> controlador = NULL;
};

#endif // ADAPTADORCONTROLADORDAO_H
