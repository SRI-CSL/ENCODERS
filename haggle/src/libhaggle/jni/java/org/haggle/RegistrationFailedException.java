package org.haggle;

        public class RegistrationFailedException extends Exception {
        private int err;

                RegistrationFailedException(String msg, int err)
                {
                        super(msg);
            this.err = err;
                }
        public int getError()
        {
            return err;
        }
        }

