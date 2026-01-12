CREATE OR REPLACE PROCEDURE create_order_simple(
    p_user_id INTEGER,
    p_product_id INTEGER,
    p_quantity INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_order_id INTEGER;
    v_price NUMERIC(10,2);
    v_stock INTEGER;
BEGIN
    SELECT price, stock_quantity INTO v_price, v_stock
    FROM products 
    WHERE product_id = p_product_id;
    
    IF v_price IS NULL THEN
        RAISE EXCEPTION 'Товар не найден';
    END IF;
    
    IF v_stock < p_quantity THEN
        RAISE EXCEPTION 'Недостаточно товара на складе. Доступно: %', v_stock;
    END IF;
    
    INSERT INTO orders(user_id, status, total_price)
    VALUES (p_user_id, 'pending', 0)
    RETURNING order_id INTO v_order_id;
    
    INSERT INTO order_items(order_id, product_id, quantity, price)
    VALUES (v_order_id, p_product_id, p_quantity, v_price);
    
    UPDATE orders
    SET total_price = p_quantity * v_price
    WHERE order_id = v_order_id;
    
    UPDATE products
    SET stock_quantity = stock_quantity - p_quantity
    WHERE product_id = p_product_id;
    
    INSERT INTO audit_log(entity_type, entity_id, operation, performed_by)
    VALUES ('order', v_order_id, 'insert', p_user_id);
    
    INSERT INTO order_status_history(order_id, old_status, new_status, changed_by)
    VALUES (v_order_id, NULL, 'pending', p_user_id);
    
    RAISE NOTICE 'Заказ % создан успешно', v_order_id;
END;
$$;

CREATE OR REPLACE PROCEDURE update_order_status_simple(
    p_order_id INTEGER,
    p_new_status VARCHAR,
    p_changed_by INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_old_status VARCHAR;
BEGIN
    SELECT status INTO v_old_status
    FROM orders
    WHERE order_id = p_order_id;

    IF v_old_status IS NULL THEN
        RAISE EXCEPTION 'Заказ % не найден', p_order_id;
    END IF;

    UPDATE orders
    SET status = p_new_status
    WHERE order_id = p_order_id;
    
    INSERT INTO order_status_history(order_id, old_status, new_status, changed_by)
    VALUES (p_order_id, v_old_status, p_new_status, p_changed_by);
    
    INSERT INTO audit_log(entity_type, entity_id, operation, performed_by)
    VALUES ('order', p_order_id, 'update', p_changed_by);
    
    RAISE NOTICE 'Статус заказа % изменен с % на %', p_order_id, v_old_status, p_new_status;
END;
$$;

CREATE OR REPLACE PROCEDURE add_to_order_simple(
    p_order_id INTEGER,
    p_product_id INTEGER,
    p_quantity INTEGER,
    p_user_id INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_price NUMERIC(10,2);
    v_order_user_id INTEGER;
    v_status VARCHAR;
BEGIN
    SELECT user_id, status INTO v_order_user_id, v_status
    FROM orders
    WHERE order_id = p_order_id;

    IF v_order_user_id IS NULL THEN
        RAISE EXCEPTION 'Заказ % не найден', p_order_id;
    END IF;
    
    IF v_status != 'pending' THEN
        RAISE EXCEPTION 'Нельзя изменить заказ в статусе %', v_status;
    END IF;

    SELECT price INTO v_price
    FROM products
    WHERE product_id = p_product_id;

    IF v_price IS NULL THEN
        RAISE EXCEPTION 'Товар % не найден', p_product_id;
    END IF;

    INSERT INTO order_items(order_id, product_id, quantity, price)
    VALUES (p_order_id, p_product_id, p_quantity, v_price);

    UPDATE orders
    SET total_price = (
        SELECT COALESCE(SUM(quantity * price), 0)
        FROM order_items
        WHERE order_id = p_order_id
    )
    WHERE order_id = p_order_id;

    INSERT INTO audit_log(entity_type, entity_id, operation, performed_by)
    VALUES ('order', p_order_id, 'update', p_user_id);
END;
$$;

CREATE OR REPLACE PROCEDURE pay_order_simple(
    p_order_id INTEGER,
    p_user_id INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_status VARCHAR;
    v_order_user_id INTEGER;
BEGIN
    SELECT status, user_id INTO v_status, v_order_user_id
    FROM orders
    WHERE order_id = p_order_id;

    IF v_status IS NULL THEN
        RAISE EXCEPTION 'Заказ % не найден', p_order_id;
    END IF;

    IF v_status != 'pending' THEN
        RAISE EXCEPTION 'Заказ % нельзя оплатить (текущий статус: %)', p_order_id, v_status;
    END IF;

    UPDATE orders
    SET status = 'completed'
    WHERE order_id = p_order_id;
    
    INSERT INTO order_status_history(order_id, old_status, new_status, changed_by)
    VALUES (p_order_id, 'pending', 'completed', p_user_id);
    
    INSERT INTO audit_log(entity_type, entity_id, operation, performed_by)
    VALUES ('order', p_order_id, 'update', p_user_id);

    RAISE NOTICE 'Заказ % успешно оплачен', p_order_id;
END;
$$;

CREATE OR REPLACE PROCEDURE return_order_simple(
    p_order_id INTEGER,
    p_user_id INTEGER
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_order_date TIMESTAMP;
    v_status VARCHAR;
    v_order_user_id INTEGER;
BEGIN
    SELECT order_date, status, user_id INTO v_order_date, v_status, v_order_user_id
    FROM orders
    WHERE order_id = p_order_id;

    IF v_status IS NULL THEN
        RAISE EXCEPTION 'Заказ % не найден', p_order_id;
    END IF;

    IF v_status != 'completed' THEN
        RAISE EXCEPTION 'Заказ % не завершен, возврат невозможен (текущий статус: %)', p_order_id, v_status;
    END IF;

    IF v_order_date < NOW() - INTERVAL '30 days' THEN
        RAISE EXCEPTION 'Прошло более 30 дней с момента заказа, возврат невозможен (заказ %)', p_order_id;
    END IF;

    UPDATE orders
    SET status = 'returned'
    WHERE order_id = p_order_id;
    
    INSERT INTO order_status_history(order_id, old_status, new_status, changed_by)
    VALUES (p_order_id, 'completed', 'returned', p_user_id);
    
    INSERT INTO audit_log(entity_type, entity_id, operation, performed_by)
    VALUES ('order', p_order_id, 'update', p_user_id);

    RAISE NOTICE 'Заказ % успешно возвращен', p_order_id;
END;
$$;